#include "mod-ollama-chat_memory.h"
#include "mod-ollama-chat_api.h"
#include "mod-ollama-chat_config.h"
#include "mod-ollama-chat_dispatch.h"
#include "mod-ollama-chat_handler.h"
#include "mod-ollama-chat_personality.h"
#include "mod-ollama-chat_roleplay.h"
#include "mod-ollama-chat-utilities.h"
#include "mod-ollama-chat_coa.h"
#include "mod-ollama-chat_httpclient.h"
#include "mod-ollama-chat_world.h"

#include "DatabaseEnv.h"
#include "Log.h"
#include "ObjectAccessor.h"
#include "Player.h"

#include "PlayerbotAI.h"
#include "PlayerbotMgr.h"

#include <fmt/core.h>

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <nlohmann/json.hpp>
#include <cctype>
#include <ctime>
#include <mutex>
#include <unordered_map>

namespace
{
    std::mutex g_mutex;

    struct OllamaMemoryState
    {
        std::vector<BotMemoryEntry>                    memories;
        std::unordered_map<uint64_t, BotRelationship>  relationships;
        bool condensing = false;    // a job is already in flight
        std::unordered_map<uint64_t, bool> relationshipPending;
        bool dirty = false;

        // Reflection and goals (Conquest of Azeroth, after "Generative Agents").
        uint32_t importanceSinceReflection = 0;
        uint64_t lastReflection = 0;
        bool     reflecting = false;
        bool     goalsPending = false;
    };

    std::unordered_map<uint64_t, OllamaMemoryState> g_state;

    std::string Escape(std::string v)
    {
        CharacterDatabase.EscapeString(v);
        return v;
    }

    uint64_t NowSeconds()
    {
        return static_cast<uint64_t>(time(nullptr));
    }

    // Count whole-word, case-insensitive occurrences of `name` in `text`.
    uint32_t CountMentions(const std::string& text, const std::string& name)
    {
        if (name.empty() || text.empty())
            return 0;

        auto lower = [](std::string v)
        {
            for (char& c : v)
                c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
            return v;
        };

        const std::string hay = lower(text);
        const std::string needle = lower(name);

        uint32_t count = 0;
        size_t pos = 0;
        while ((pos = hay.find(needle, pos)) != std::string::npos)
        {
            const bool startOk = (pos == 0) ||
                                 !std::isalnum(static_cast<unsigned char>(hay[pos - 1]));
            const size_t end = pos + needle.size();
            const bool endOk = (end >= hay.size()) ||
                               !std::isalnum(static_cast<unsigned char>(hay[end]));
            if (startOk && endOk)
                ++count;
            pos = end;
        }
        return count;
    }

    // Total tokens currently held in a bot's raw conversation history.
    uint32_t HistoryTokens(uint64_t botGuid)
    {
        uint32_t total = 0;

        std::lock_guard<std::mutex> lock(g_ConversationHistoryMutex);
        auto botIt = g_BotConversationHistory.find(botGuid);
        if (botIt == g_BotConversationHistory.end())
            return 0;

        for (const auto& [playerGuid, history] : botIt->second)
            for (const auto& pair : history)
                total += Memory_EstimateTokens(pair.playerMessage) + Memory_EstimateTokens(pair.botReply);

        return total;
    }

    std::string RenderHistory(uint64_t botGuid)
    {
        std::string out;

        std::lock_guard<std::mutex> lock(g_ConversationHistoryMutex);
        auto botIt = g_BotConversationHistory.find(botGuid);
        if (botIt == g_BotConversationHistory.end())
            return out;

        for (const auto& [playerGuid, history] : botIt->second)
        {
            Player* other = ObjectAccessor::FindPlayer(ObjectGuid(playerGuid));
            const std::string name = other ? other->GetName() : "someone";

            for (const auto& pair : history)
            {
                if (!pair.playerMessage.empty())
                    out += name + ": " + pair.playerMessage + "\n";
                if (!pair.botReply.empty())
                    out += "You: " + pair.botReply + "\n";
            }
        }
        return out;
    }

    void ClearHistory(uint64_t botGuid)
    {
        {
            std::lock_guard<std::mutex> lock(g_ConversationHistoryMutex);
            g_BotConversationHistory.erase(botGuid);
        }

        // The rows outlived the in-memory window until now, so every restart
        // reloaded a conversation that had already been condensed and paid to
        // condense it all over again.
        DeleteBotConversationHistoryFromDB(botGuid);
    }

    // Parse the model's condensation output. One memory per line, optionally
    // prefixed with an importance score: "7 | He saved my life at the bridge."
    std::vector<BotMemoryEntry> ParseMemories(const std::string& text)
    {
        std::vector<BotMemoryEntry> out;

        size_t start = 0;
        while (start <= text.size())
        {
            size_t end = text.find('\n', start);
            if (end == std::string::npos)
                end = text.size();

            std::string line = text.substr(start, end - start);
            start = end + 1;

            // Trim and strip list markers the model adds unasked.
            size_t a = line.find_first_not_of(" \t\r-*•");
            if (a == std::string::npos)
                continue;
            size_t b = line.find_last_not_of(" \t\r");
            line = line.substr(a, b - a + 1);
            if (line.empty())
                continue;

            BotMemoryEntry entry;
            entry.importance = 5;

            // Leading "N |" or "N."
            size_t sep = line.find('|');
            if (sep != std::string::npos && sep <= 3)
            {
                try
                {
                    const int score = std::stoi(line.substr(0, sep));
                    entry.importance = static_cast<uint8_t>(std::clamp(score, 1, 10));
                    line = line.substr(sep + 1);
                }
                catch (const std::exception&) { }
            }

            a = line.find_first_not_of(" \t");
            if (a == std::string::npos)
                continue;
            line = line.substr(a);

            if (line.size() < 8)     // not a real memory
                continue;

            entry.text      = line;
            entry.createdAt = NowSeconds();
            entry.lastAccess = entry.createdAt;
            out.push_back(std::move(entry));

            if (out.size() >= 12)    // sanity cap on one condensation pass
                break;
        }

        return out;
    }
}

// --------------------------------------------------------------------------

uint32_t Memory_EstimateTokens(const std::string& text)
{
    return static_cast<uint32_t>((text.size() + 3) / 4);
}

void Memory_Load()
{
    std::lock_guard<std::mutex> lock(g_mutex);
    g_state.clear();

    if (QueryResult result = CharacterDatabase.Query(
            "SELECT bot_guid, memory_text, importance, UNIX_TIMESTAMP(created_at), about_guid, kind "
            "FROM mod_ollama_chat_memories ORDER BY importance DESC"))
    {
        uint32_t loaded = 0;
        do
        {
            Field* f = result->Fetch();
            BotMemoryEntry e;
            e.text       = f[1].Get<std::string>();
            e.importance = f[2].Get<uint8>();
            e.createdAt  = f[3].Get<uint64>();
            e.aboutGuid  = f[4].Get<uint64>();
            e.kind       = f[5].Get<uint8>();
            e.lastAccess = e.createdAt;

            if (!e.text.empty())
            {
                g_state[f[0].Get<uint64>()].memories.push_back(std::move(e));
                ++loaded;
            }
        } while (result->NextRow());

        LOG_INFO("module.ollamachat", "[Ollama Chat] Loaded {} bot memories.", loaded);
    }

    if (QueryResult result = CharacterDatabase.Query(
            "SELECT bot_guid, other_guid, other_name, description, mentions, "
            "UNIX_TIMESTAMP(updated_at) FROM mod_ollama_chat_relationships"))
    {
        uint32_t loaded = 0;
        do
        {
            Field* f = result->Fetch();
            BotRelationship r;
            r.otherGuid   = f[1].Get<uint64>();
            r.otherName   = f[2].Get<std::string>();
            r.description = f[3].Get<std::string>();
            r.mentions    = f[4].Get<uint32>();
            r.updatedAt   = f[5].Get<uint64>();

            g_state[f[0].Get<uint64>()].relationships[r.otherGuid] = std::move(r);
            ++loaded;
        } while (result->NextRow());

        LOG_INFO("module.ollamachat", "[Ollama Chat] Loaded {} bot relationships.", loaded);
    }
}

void Memory_SaveAll()
{
    std::lock_guard<std::mutex> lock(g_mutex);

    for (auto& [botGuid, state] : g_state)
    {
        if (!state.dirty)
            continue;

        // The delete and the reinserts have to land together. As separate
        // async statements, a crash between them left the bot with no memories
        // at all, which is worse than a stale set.
        CharacterDatabaseTransaction trans = CharacterDatabase.BeginTransaction();

        trans->Append(SafeFormat(
            "DELETE FROM mod_ollama_chat_memories WHERE bot_guid = {}", botGuid));

        std::string values;
        for (const BotMemoryEntry& m : state.memories)
        {
            if (!values.empty())
                values += ',';

            values += SafeFormat("({}, '{}', {}, FROM_UNIXTIME({}), {}, {})",
                                 botGuid, Escape(m.text), uint32_t(m.importance), m.createdAt, m.aboutGuid,
                                 uint32_t(m.kind));
        }

        if (!values.empty())
        {
            trans->Append("INSERT INTO mod_ollama_chat_memories "
                          "(bot_guid, memory_text, importance, created_at, about_guid, kind) VALUES " + values);
        }

        for (const auto& [otherGuid, r] : state.relationships)
        {
            if (r.description.empty())
                continue;

            // ON DUPLICATE KEY UPDATE rather than REPLACE INTO: REPLACE is a
            // delete plus an insert, so it rewrites the row and every index
            // entry even when nothing about the relationship changed.
            trans->Append(SafeFormat(
                "INSERT INTO mod_ollama_chat_relationships "
                "(bot_guid, other_guid, other_name, description, mentions, updated_at) "
                "VALUES ({}, {}, '{}', '{}', {}, FROM_UNIXTIME({})) "
                "ON DUPLICATE KEY UPDATE other_name = VALUES(other_name), "
                "description = VALUES(description), mentions = VALUES(mentions), "
                "updated_at = VALUES(updated_at)",
                botGuid, otherGuid, Escape(r.otherName), Escape(r.description),
                r.mentions, r.updatedAt ? r.updatedAt : NowSeconds()));
        }

        CharacterDatabase.CommitTransaction(trans);

        state.dirty = false;
    }
}

void Memory_ForgetBot(ObjectGuid botGuid)
{
    // Memories persist in the database; this only drops the in-memory copy for
    // a character who has logged out.
    std::lock_guard<std::mutex> lock(g_mutex);
    g_state.erase(botGuid.GetRawValue());
}

// --------------------------------------------------------------------------

void Memory_NoteExchange(uint64_t botGuid, uint64_t otherGuid,
                         const std::string& otherName,
                         const std::string& incomingMessage,
                         const std::string& botReply)
{
    if (!g_MemoryEnable || botGuid == 0)
        return;

    Player* bot = ObjectAccessor::FindPlayer(ObjectGuid(botGuid));
    if (!bot)
        return;

    // ---- relationship mention counting ---------------------------------
    if (g_RelationshipEnable && otherGuid != 0 && !otherName.empty())
    {
        const std::string combined = incomingMessage + " " + botReply;
        const uint32_t hits = 1 + CountMentions(combined, otherName);

        bool trigger = false;
        {
            std::lock_guard<std::mutex> lock(g_mutex);
            OllamaMemoryState& state = g_state[botGuid];
            BotRelationship& rel = state.relationships[otherGuid];

            rel.otherGuid = otherGuid;
            if (rel.otherName.empty())
                rel.otherName = otherName;
            rel.mentions += hits;
            state.dirty = true;

            if (rel.mentions >= g_RelationshipMentionThreshold &&
                !state.relationshipPending[otherGuid])
            {
                state.relationshipPending[otherGuid] = true;
                rel.mentions = 0;      // reset the counter for the next revision
                trigger = true;
            }
        }

        if (trigger)
        {
            std::string prompt = Memory_BuildRelationshipPrompt(bot, otherGuid, otherName);
            if (!prompt.empty())
                OllamaDispatch_SubmitRelationship(botGuid, otherGuid, otherName, prompt);
            else
            {
                std::lock_guard<std::mutex> lock(g_mutex);
                g_state[botGuid].relationshipPending[otherGuid] = false;
            }
        }
    }

    // ---- condensation ---------------------------------------------------
    if (g_MemoryHistoryTokenLimit == 0)
        return;

    if (HistoryTokens(botGuid) < g_MemoryHistoryTokenLimit)
        return;

    {
        std::lock_guard<std::mutex> lock(g_mutex);
        OllamaMemoryState& state = g_state[botGuid];
        if (state.condensing)
            return;
        state.condensing = true;
    }

    std::string prompt = Memory_BuildCondensationPrompt(bot);
    if (prompt.empty())
    {
        std::lock_guard<std::mutex> lock(g_mutex);
        g_state[botGuid].condensing = false;
        return;
    }

    OllamaDispatch_SubmitCondensation(botGuid, prompt);
}

// --------------------------------------------------------------------------

std::string Memory_BuildCondensationPrompt(Player* bot)
{
    if (!bot || g_MemoryCondensePrompt.empty())
        return "";

    const std::string history = RenderHistory(bot->GetGUID().GetRawValue());
    if (history.empty())
        return "";

    return SafeFormat(g_MemoryCondensePrompt,
                      fmt::arg("bot_name", bot->GetName()),
                      fmt::arg("history", history));
}

std::string Memory_BuildRelationshipPrompt(Player* bot, uint64_t otherGuid,
                                           const std::string& otherName)
{
    if (!bot || g_RelationshipUpdatePrompt.empty())
        return "";

    std::string existing;
    {
        std::lock_guard<std::mutex> lock(g_mutex);
        auto it = g_state.find(bot->GetGUID().GetRawValue());
        if (it != g_state.end())
        {
            auto relIt = it->second.relationships.find(otherGuid);
            if (relIt != it->second.relationships.end())
                existing = relIt->second.description;
        }
    }

    const std::string history = RenderHistory(bot->GetGUID().GetRawValue());

    return SafeFormat(g_RelationshipUpdatePrompt,
                      fmt::arg("bot_name", bot->GetName()),
                      fmt::arg("other_name", otherName),
                      fmt::arg("existing", existing.empty() ? "nothing yet" : existing),
                      fmt::arg("history", history));
}

// --------------------------------------------------------------------------

std::string Memory_BuildPromptSection(Player* bot, Player* about)
{
    if (!g_MemoryEnable || !bot)
        return "";

    const uint64_t botGuid   = bot->GetGUID().GetRawValue();
    const uint64_t aboutGuid = about ? about->GetGUID().GetRawValue() : 0;

    std::vector<BotMemoryEntry> memories;
    std::vector<BotRelationship> relationships;

    {
        std::lock_guard<std::mutex> lock(g_mutex);
        auto it = g_state.find(botGuid);
        if (it == g_state.end())
            return "";

        memories = it->second.memories;
        for (const auto& [guid, rel] : it->second.relationships)
            if (!rel.description.empty())
                relationships.push_back(rel);
    }

    std::string out;

    // ---- relationships: whoever we are talking to comes first ----------
    if (g_RelationshipEnable && !relationships.empty())
    {
        std::sort(relationships.begin(), relationships.end(),
                  [aboutGuid](const BotRelationship& a, const BotRelationship& b)
                  {
                      if ((a.otherGuid == aboutGuid) != (b.otherGuid == aboutGuid))
                          return a.otherGuid == aboutGuid;
                      return a.updatedAt > b.updatedAt;
                  });

        std::string lines;
        uint32_t taken = 0;
        for (const BotRelationship& r : relationships)
        {
            if (g_RelationshipMaxPerPrompt > 0 && taken >= g_RelationshipMaxPerPrompt)
                break;
            lines += " - " + r.otherName + ": " + r.description + "\n";
            ++taken;
        }

        if (!lines.empty() && !g_RelationshipPromptTemplate.empty())
            out += SafeFormat(g_RelationshipPromptTemplate, fmt::arg("relationships", lines));
    }

    // ---- what the bot went through with this person, most recent first ----
    auto ago = [](uint64_t at) -> std::string
    {
        const uint64_t now = NowSeconds();
        const uint64_t d = now > at ? now - at : 0;
        if (d < 2 * 3600)       return "just now";
        if (d < 24 * 3600)      return "earlier today";
        if (d < 2 * 24 * 3600)  return "yesterday";
        if (d < 7 * 24 * 3600)  return "a few days ago";
        if (d < 30 * 24 * 3600) return "some weeks ago";
        return "long ago";
    };

    std::vector<BotMemoryEntry> shared, own, notes;
    for (const BotMemoryEntry& m : memories)
    {
        if (aboutGuid && m.aboutGuid == aboutGuid)
            shared.push_back(m);
        else if (m.aboutGuid == botGuid)
            own.push_back(m);
        else if (m.aboutGuid == 0)
            notes.push_back(m);
    }
    auto newest = [](const BotMemoryEntry& a, const BotMemoryEntry& b) { return a.createdAt > b.createdAt; };

    if (!shared.empty() && about)
    {
        std::sort(shared.begin(), shared.end(), newest);
        out += " What you have been through with " + std::string(about->GetName()) + ":";
        for (size_t i = 0; i < shared.size() && i < 5; ++i)
            out += " " + shared[i].text + " (" + ago(shared[i].createdAt) + ")";
    }
    if (!own.empty())
    {
        std::sort(own.begin(), own.end(), newest);
        out += " What you did lately:";
        for (size_t i = 0; i < own.size() && i < 3; ++i)
            out += " " + own[i].text + " (" + ago(own[i].createdAt) + ")";
    }
    memories = std::move(notes);

    // ---- memories: most important first, inside a token budget ---------
    if (!memories.empty() && !g_MemoryPromptTemplate.empty())
    {
        std::sort(memories.begin(), memories.end(),
                  [](const BotMemoryEntry& a, const BotMemoryEntry& b)
                  {
                      if (a.importance != b.importance)
                          return a.importance > b.importance;
                      return a.createdAt > b.createdAt;
                  });

        std::string lines;
        uint32_t used = 0;
        for (const BotMemoryEntry& m : memories)
        {
            const uint32_t cost = Memory_EstimateTokens(m.text) + 4;
            if (g_MemoryPromptTokenBudget > 0 && used + cost > g_MemoryPromptTokenBudget)
                continue;      // try the next, shorter one rather than stopping
            lines += " - " + m.text + "\n";
            used += cost;
        }

        if (!lines.empty())
            out += SafeFormat(g_MemoryPromptTemplate, fmt::arg("memories", lines));
    }

    return out;
}

// --------------------------------------------------------------------------
// Worker-side
// --------------------------------------------------------------------------

void Memory_RunCondensation(uint64_t botGuid, const std::string& prompt)
{
    OllamaApiResult api = QueryOllama(prompt, OllamaRequestKind::Sentiment);

    std::vector<BotMemoryEntry> fresh;
    if (api.ok)
        fresh = ParseMemories(api.text);

    {
        std::lock_guard<std::mutex> lock(g_mutex);
        OllamaMemoryState& state = g_state[botGuid];
        state.condensing = false;

        if (!fresh.empty())
        {
            for (const BotMemoryEntry& m : fresh)
                state.importanceSinceReflection += m.importance;
            state.memories.insert(state.memories.end(), fresh.begin(), fresh.end());

            // Keep the most important, drop the rest. Goals are never pushed out.
            std::sort(state.memories.begin(), state.memories.end(),
                      [](const BotMemoryEntry& a, const BotMemoryEntry& b)
                      {
                          if ((a.kind == MEMORY_GOAL) != (b.kind == MEMORY_GOAL))
                              return a.kind == MEMORY_GOAL;
                          if (a.importance != b.importance)
                              return a.importance > b.importance;
                          return a.createdAt > b.createdAt;
                      });

            if (g_MemoryMaxPerBot > 0 && state.memories.size() > g_MemoryMaxPerBot)
                state.memories.resize(g_MemoryMaxPerBot);

            state.dirty = true;
        }
    }

    if (!api.ok)
    {
        if (g_DebugEnabled)
            LOG_INFO("module.ollamachat",
                     "[Ollama Chat] Memory condensation failed for bot {}: {}",
                     botGuid, api.error);
        return;      // keep the history; we will try again next time
    }

    // The history has been distilled; drop it so the window starts fresh.
    ClearHistory(botGuid);

    if (g_DebugEnabled)
        LOG_INFO("module.ollamachat",
                 "[Ollama Chat] Condensed history for bot {} into {} memories.",
                 botGuid, fresh.size());
}

void Memory_RunRelationshipUpdate(uint64_t botGuid, uint64_t otherGuid,
                                  const std::string& otherName,
                                  const std::string& prompt)
{
    OllamaApiResult api = QueryOllama(prompt, OllamaRequestKind::Sentiment);

    std::string description;
    if (api.ok)
    {
        description = api.text;

        // One sentence, no markup, bounded length.
        for (char& c : description)
            if (c == '\n' || c == '\r' || c == '\t')
                c = ' ';

        const size_t cut = description.find_first_of(" \t");
        (void)cut;

        if (description.size() > g_RelationshipMaxLength)
        {
            description.resize(g_RelationshipMaxLength);
            const size_t lastSpace = description.find_last_of(' ');
            if (lastSpace != std::string::npos && lastSpace > g_RelationshipMaxLength / 2)
                description.erase(lastSpace);
        }
    }

    std::lock_guard<std::mutex> lock(g_mutex);
    OllamaMemoryState& state = g_state[botGuid];
    state.relationshipPending[otherGuid] = false;

    if (description.empty())
        return;

    BotRelationship& rel = state.relationships[otherGuid];
    rel.otherGuid   = otherGuid;
    rel.otherName   = otherName;
    rel.description = std::move(description);
    rel.updatedAt   = NowSeconds();
    state.dirty     = true;

    if (g_DebugEnabled)
        LOG_INFO("module.ollamachat",
                 "[Ollama Chat] Bot {} relationship with {} updated: {}",
                 botGuid, otherName, rel.description);
}

// --------------------------------------------------------------------------

void Memory_AddExperience(uint64_t botGuid, uint64_t aboutGuid, const std::string& text, uint8_t importance,
                          uint8_t kind)
{
    if (!g_MemoryEnable || botGuid == 0 || text.empty())
        return;

    std::lock_guard<std::mutex> lock(g_mutex);
    OllamaMemoryState& state = g_state[botGuid];

    // The same thing twice within the hour is one experience ("killed a Defias Pillager" for every pillager).
    const uint64_t now = NowSeconds();
    for (const BotMemoryEntry& m : state.memories)
        if (m.aboutGuid == aboutGuid && m.text == text && m.createdAt + 3600 > now)
            return;

    BotMemoryEntry e;
    e.text       = text;
    e.importance = std::clamp<uint8_t>(importance, 1, 10);
    e.createdAt  = now;
    e.aboutGuid  = aboutGuid;
    e.kind       = kind;
    e.lastAccess = now;
    state.memories.push_back(std::move(e));
    if (kind != MEMORY_GOAL)
        state.importanceSinceReflection += importance;

    // Over the limit: the least important and, among those, the oldest go first. Goals are never pushed out.
    if (g_MemoryMaxPerBot > 0 && state.memories.size() > g_MemoryMaxPerBot)
    {
        std::stable_sort(state.memories.begin(), state.memories.end(),
                  [](const BotMemoryEntry& a, const BotMemoryEntry& b)
                  {
                      if ((a.kind == MEMORY_GOAL) != (b.kind == MEMORY_GOAL))
                          return a.kind == MEMORY_GOAL;
                      if (a.importance != b.importance)
                          return a.importance > b.importance;
                      return a.createdAt > b.createdAt;
                  });
        state.memories.resize(g_MemoryMaxPerBot);
    }
    state.dirty = true;
}

// ==========================================================================
// Conquest of Azeroth: recall, reflection, goals and rumours, after
// "Generative Agents: Interactive Simulacra of Human Behavior" (Park et al., 2023).
// ==========================================================================

namespace
{
    // ---- relevance ---------------------------------------------------------

    std::vector<std::string> Words(const std::string& text)
    {
        static const std::unordered_map<std::string, bool> stop = {
            {"the",1},{"and",1},{"you",1},{"your",1},{"with",1},{"for",1},{"that",1},{"this",1},{"was",1},{"are",1},
            {"have",1},{"has",1},{"had",1},{"but",1},{"not",1},{"what",1},{"who",1},{"how",1},{"his",1},{"her",1},
            {"they",1},{"them",1},{"from",1},{"about",1},{"there",1},{"here",1},{"just",1},{"into",1},{"will",1},
            {"would",1},{"can",1},{"could",1},{"did",1},{"does",1},{"its",1},{"our",1},{"out",1},{"all",1},
            {"der",1},{"die",1},{"das",1},{"und",1},{"ich",1},{"du",1},{"nicht",1},{"ein",1},{"eine",1},{"ist",1},
        };
        std::vector<std::string> out;
        std::string w;
        auto flush = [&]()
        {
            if (w.size() >= 3 && !stop.count(w))
            {
                // A crude stem: "worgens" and "worgen", "fought" stays, "killed" and "kill".
                if (w.size() > 4 && w.back() == 's') w.pop_back();
                if (w.size() > 5 && w.compare(w.size() - 2, 2, "ed") == 0) w.resize(w.size() - 2);
                out.push_back(w);
            }
            w.clear();
        };
        for (char ch : text)
        {
            unsigned char c = static_cast<unsigned char>(ch);
            if (std::isalnum(c) || c == '\'' || (c & 0x80))
                w += static_cast<char>(std::tolower(c));
            else
                flush();
        }
        flush();
        return out;
    }

    float KeywordRelevance(const std::vector<std::string>& query, const std::string& text)
    {
        if (query.empty())
            return 0.0f;
        std::vector<std::string> words = Words(text);
        if (words.empty())
            return 0.0f;
        uint32_t hits = 0;
        for (const std::string& q : query)
            if (std::find(words.begin(), words.end(), q) != words.end())
                ++hits;
        return float(hits) / std::sqrt(float(query.size()) * float(words.size()));
    }

    // ---- embeddings (optional) --------------------------------------------

    std::mutex g_embedMutex;
    std::unordered_map<std::string, std::vector<float>> g_embedCache;
    time_t g_embedBrokenUntil = 0;

    // Embeddings of `texts` from Ollama's /api/embed, cached by text. Empty when there is no embedding model, the
    // provider is not Ollama, or the call failed (then for ten minutes it is not tried again).
    std::vector<std::vector<float>> Embed(const std::vector<std::string>& texts)
    {
        std::string model = g_RecallEmbedModel;
        if (model.empty() || g_OllamaProvider != OllamaProvider::Ollama || texts.empty())
            return {};
        {
            std::lock_guard<std::mutex> lock(g_embedMutex);
            if (time(nullptr) < g_embedBrokenUntil)
                return {};
        }

        std::vector<std::vector<float>> out(texts.size());
        std::vector<size_t> missing;
        {
            std::lock_guard<std::mutex> lock(g_embedMutex);
            for (size_t i = 0; i < texts.size(); ++i)
            {
                auto it = g_embedCache.find(texts[i]);
                if (it != g_embedCache.end())
                    out[i] = it->second;
                else
                    missing.push_back(i);
            }
        }
        if (missing.empty())
            return out;

        nlohmann::json body;
        body["model"] = model;
        body["input"] = nlohmann::json::array();
        for (size_t i : missing)
            body["input"].push_back(texts[i]);

        thread_local OllamaHttpClient client;
        OllamaHttpResult http = client.PostEx(OllamaDeriveBaseUrl(g_OllamaUrl) + "/api/embed", body.dump(), 30);
        bool ok = http.ok();
        nlohmann::json reply;
        if (ok)
        {
            reply = nlohmann::json::parse(http.body, nullptr, false);
            ok = !reply.is_discarded() && reply.contains("embeddings") && reply["embeddings"].is_array() &&
                 reply["embeddings"].size() == missing.size();
        }
        if (!ok)
        {
            std::lock_guard<std::mutex> lock(g_embedMutex);
            g_embedBrokenUntil = time(nullptr) + 600;
            LOG_WARN("module.ollamachat", "[Ollama Chat] The embedding model '{}' did not answer ({}); memories are "
                     "chosen by keywords for the next ten minutes. Is it pulled (ollama pull {})?",
                     model, http.status ? std::to_string(http.status) : http.error, model);
            return {};
        }

        std::lock_guard<std::mutex> lock(g_embedMutex);
        if (g_embedCache.size() > 50000)
            g_embedCache.clear();
        for (size_t k = 0; k < missing.size(); ++k)
        {
            std::vector<float> v = reply["embeddings"][k].get<std::vector<float>>();
            out[missing[k]] = v;
            g_embedCache[texts[missing[k]]] = std::move(v);
        }
        return out;
    }

    float Cosine(const std::vector<float>& a, const std::vector<float>& b)
    {
        if (a.empty() || a.size() != b.size())
            return 0.0f;
        double dot = 0, na = 0, nb = 0;
        for (size_t i = 0; i < a.size(); ++i)
        {
            dot += double(a[i]) * b[i];
            na += double(a[i]) * a[i];
            nb += double(b[i]) * b[i];
        }
        return (na > 0 && nb > 0) ? float(dot / std::sqrt(na * nb)) : 0.0f;
    }

    // ---- scoring -------------------------------------------------------------

    struct Scored
    {
        const BotMemoryEntry* memory;
        float score;
    };

    // score = recency + importance + relevance, each scaled to 0..1 over the candidates (the paper's min-max),
    // recency decaying by 0.995 an hour since the memory last came up.
    std::vector<Scored> Rank(const std::vector<const BotMemoryEntry*>& candidates, const std::string& query)
    {
        std::vector<Scored> out;
        if (candidates.empty())
            return out;

        const uint64_t now = NowSeconds();
        std::vector<float> recency, importance, relevance;
        const std::vector<std::string> queryWords = Words(query);

        std::vector<std::vector<float>> vectors;
        std::vector<float> queryVector;
        if (!query.empty())
        {
            std::vector<std::string> texts;
            texts.push_back(query);
            for (const BotMemoryEntry* m : candidates)
                texts.push_back(m->text);
            vectors = Embed(texts);
            if (!vectors.empty())
                queryVector = vectors[0];
        }

        for (size_t i = 0; i < candidates.size(); ++i)
        {
            const BotMemoryEntry* m = candidates[i];
            const uint64_t from = std::max(m->lastAccess, m->createdAt);
            const double hours = now > from ? double(now - from) / 3600.0 : 0.0;
            recency.push_back(float(std::pow(0.995, hours)));
            importance.push_back(float(m->importance));
            relevance.push_back(!queryVector.empty() ? Cosine(queryVector, vectors[i + 1])
                                                     : KeywordRelevance(queryWords, m->text));
        }

        auto scale = [](std::vector<float>& v)
        {
            auto [lo, hi] = std::minmax_element(v.begin(), v.end());
            const float a = *lo, b = *hi;
            for (float& x : v)
                x = b > a ? (x - a) / (b - a) : (b > 0 ? 1.0f : 0.0f);
        };
        scale(recency);
        scale(importance);
        scale(relevance);

        for (size_t i = 0; i < candidates.size(); ++i)
            out.push_back({ candidates[i], recency[i] + importance[i] + relevance[i] });
        std::stable_sort(out.begin(), out.end(), [](const Scored& a, const Scored& b) { return a.score > b.score; });
        return out;
    }

    std::string Ago(uint64_t at)
    {
        const uint64_t now = NowSeconds();
        const uint64_t d = now > at ? now - at : 0;
        if (d < 2 * 3600)       return "just now";
        if (d < 24 * 3600)      return "earlier today";
        if (d < 2 * 24 * 3600)  return "yesterday";
        if (d < 7 * 24 * 3600)  return "a few days ago";
        if (d < 30 * 24 * 3600) return "some weeks ago";
        return "long ago";
    }

    const char* MARKER_OPEN  = "\x01RECALL|";
    const char* MARKER_CLOSE = "\x01";

    std::string Clean(std::string v)
    {
        for (char& c : v)
            if (c == '\x01' || c == '|' || c == '\n' || c == '\r')
                c = ' ';
        return v;
    }

    // The memory section for one bot, about one person (0 for none), for one moment (`query`).
    std::string Recall(uint64_t botGuid, uint64_t aboutGuid, const std::string& aboutName, const std::string& query)
    {
        std::vector<BotMemoryEntry> memories;
        std::vector<BotRelationship> relationships;
        {
            std::lock_guard<std::mutex> lock(g_mutex);
            auto it = g_state.find(botGuid);
            if (it == g_state.end())
                return "";
            memories = it->second.memories;
            for (const auto& [guid, rel] : it->second.relationships)
                if (!rel.description.empty())
                    relationships.push_back(rel);
        }

        std::string out;

        // What the bot thinks of people, whoever it talks to first (the original's relationships).
        if (g_RelationshipEnable && !relationships.empty() && !g_RelationshipPromptTemplate.empty())
        {
            std::sort(relationships.begin(), relationships.end(),
                      [aboutGuid](const BotRelationship& a, const BotRelationship& b)
                      {
                          if ((a.otherGuid == aboutGuid) != (b.otherGuid == aboutGuid))
                              return a.otherGuid == aboutGuid;
                          return a.updatedAt > b.updatedAt;
                      });
            std::string lines;
            uint32_t taken = 0;
            for (const BotRelationship& r : relationships)
            {
                if (g_RelationshipMaxPerPrompt > 0 && taken >= g_RelationshipMaxPerPrompt)
                    break;
                lines += " - " + r.otherName + ": " + r.description + "\n";
                ++taken;
            }
            out += SafeFormat(g_RelationshipPromptTemplate, fmt::arg("relationships", lines));
        }

        // The query is the moment: what was said, and who to.
        const std::string moment = aboutName.empty() ? query : aboutName + " " + query;

        std::vector<const BotMemoryEntry*> person, beliefs, lately, other;
        std::vector<const BotMemoryEntry*> goals;
        for (const BotMemoryEntry& m : memories)
        {
            if (m.kind == MEMORY_GOAL)
                goals.push_back(&m);
            else if (aboutGuid && m.aboutGuid == aboutGuid)
                person.push_back(&m);
            else if (m.kind == MEMORY_REFLECTION && (m.aboutGuid == 0 || m.aboutGuid == botGuid))
                beliefs.push_back(&m);
            else if (m.kind == MEMORY_EXPERIENCE && m.aboutGuid == botGuid)
                lately.push_back(&m);
            else
                other.push_back(&m);
        }

        std::vector<std::string> used;
        uint32_t budget = g_MemoryPromptTokenBudget ? g_MemoryPromptTokenBudget : 400;
        auto take = [&](const std::vector<Scored>& ranked, size_t max, const std::string& header, bool when)
        {
            std::string lines;
            size_t n = 0;
            for (const Scored& s : ranked)
            {
                if (n >= max)
                    break;
                const uint32_t cost = Memory_EstimateTokens(s.memory->text) + 4;
                if (cost > budget)
                    continue;
                budget -= cost;
                lines += " " + s.memory->text + (when ? " (" + Ago(s.memory->createdAt) + ")" : std::string());
                used.push_back(s.memory->text);
                ++n;
            }
            if (!lines.empty())
                out += " " + header + lines;
        };

        if (!person.empty())
        {
            std::vector<const BotMemoryEntry*> lived, thought, heard;
            for (const BotMemoryEntry* m : person)
                (m->kind == MEMORY_REFLECTION ? thought : m->kind == MEMORY_RUMOUR ? heard : lived).push_back(m);
            take(Rank(thought, moment), 2, "What you think of " + aboutName + ":", false);
            take(Rank(lived, moment), 5, "What you have been through with " + aboutName + ":", true);
            take(Rank(heard, moment), 2, "What you have heard about " + aboutName + ":", false);
        }
        if (!goals.empty() && g_GoalsEnable)
        {
            std::string lines;
            for (const BotMemoryEntry* g : goals)
                lines += " " + g->text;
            out += " What you want in life (your own wishes, not what you are doing right now; bring them up when "
                   "it fits, and how you are getting on):" + lines;
        }
        take(Rank(beliefs, moment), 3, "What you have come to believe:", false);
        take(Rank(lately, moment), 3, "What you did lately:", true);
        take(Rank(other, moment), 6, "Other things you remember:", false);

        // What came up is fresh in mind again (the paper's recency counts from the last retrieval).
        if (!used.empty())
        {
            const uint64_t now = NowSeconds();
            std::lock_guard<std::mutex> lock(g_mutex);
            auto it = g_state.find(botGuid);
            if (it != g_state.end())
                for (BotMemoryEntry& m : it->second.memories)
                    if (std::find(used.begin(), used.end(), m.text) != used.end())
                        m.lastAccess = now;
        }
        return out;
    }

    // ---- reflection and goals ------------------------------------------------

    std::unordered_map<uint64_t, uint64_t> g_lastGoalAttempt;
    std::vector<uint64_t> g_reflectionTimes;    // when the last background thoughts were started, for the hourly cap

    // Lines of the model's answer, without numbering, bullets and quotes.
    std::vector<std::string> Lines(const std::string& text)
    {
        std::vector<std::string> out;
        size_t start = 0;
        while (start < text.size())
        {
            size_t end = text.find('\n', start);
            if (end == std::string::npos)
                end = text.size();
            std::string line = text.substr(start, end - start);
            start = end + 1;
            size_t a = line.find_first_not_of(" \t\r-*\xE2\x80\xA2\"");
            if (a == std::string::npos)
                continue;
            line = line.substr(a);
            while (!line.empty() && std::isdigit(static_cast<unsigned char>(line[0])))
                line.erase(0, 1);
            if (!line.empty() && (line[0] == '.' || line[0] == ')' || line[0] == ':'))
                line.erase(0, 1);
            a = line.find_first_not_of(" \t");
            if (a == std::string::npos)
                continue;
            line = line.substr(a);
            while (!line.empty() && (line.back() == ' ' || line.back() == '\r' || line.back() == '"'))
                line.pop_back();
            if (line.size() >= 8)
                out.push_back(line);
        }
        return out;
    }

    // "Chris never leaves me behind (because of 1, 4)" -> the insight alone.
    std::string StripCitation(std::string line)
    {
        size_t because = line.rfind("(because");
        if (because == std::string::npos)
            because = line.rfind("(Because");
        if (because != std::string::npos)
            line.erase(because);
        while (!line.empty() && (line.back() == ' ' || line.back() == '.'))
            line.pop_back();
        return line.empty() ? line : line + ".";
    }

    uint8_t ImportanceOf(std::string& line)
    {
        // "[7] text" or "text [7]"
        uint8_t importance = 6;
        size_t open = line.find('[');
        size_t close = open == std::string::npos ? std::string::npos : line.find(']', open);
        if (close != std::string::npos && close - open <= 3)
        {
            int v = std::atoi(line.substr(open + 1, close - open - 1).c_str());
            if (v >= 1 && v <= 10)
                importance = uint8_t(v);
            line.erase(open, close - open + 1);
            size_t a = line.find_first_not_of(' ');
            line = a == std::string::npos ? std::string() : line.substr(a);
        }
        return importance;
    }

    struct Known { uint64_t guid; std::string name; };

    void StoreReflections(uint64_t botGuid, const std::vector<std::string>& lines, const std::vector<Known>& people)
    {
        uint32_t reflections = 0;
        for (std::string line : lines)
        {
            const bool goal = line.rfind("GOAL", 0) == 0 || line.rfind("Goal", 0) == 0;
            if (goal)
            {
                size_t colon = line.find(':');
                line = colon == std::string::npos ? line.substr(4) : line.substr(colon + 1);
                size_t a = line.find_first_not_of(' ');
                if (a == std::string::npos)
                    continue;
                line = StripCitation(line.substr(a));
                if (line.size() < 8)
                    continue;
                // A new goal takes the place of the oldest when there are two.
                std::lock_guard<std::mutex> lock(g_mutex);
                OllamaMemoryState& state = g_state[botGuid];
                std::vector<size_t> have;
                for (size_t i = 0; i < state.memories.size(); ++i)
                    if (state.memories[i].kind == MEMORY_GOAL)
                        have.push_back(i);
                bool same = false;
                for (size_t i : have)
                    same = same || state.memories[i].text == line;
                if (same)
                    continue;
                if (have.size() >= 2)
                {
                    size_t oldest = have[0];
                    for (size_t i : have)
                        if (state.memories[i].createdAt < state.memories[oldest].createdAt)
                            oldest = i;
                    state.memories.erase(state.memories.begin() + oldest);
                }
                BotMemoryEntry e;
                e.text = line;
                e.importance = 8;
                e.kind = MEMORY_GOAL;
                e.aboutGuid = botGuid;
                e.createdAt = e.lastAccess = NowSeconds();
                state.memories.push_back(std::move(e));
                state.dirty = true;
                continue;
            }
            if (reflections >= 3)
                continue;
            uint8_t importance = ImportanceOf(line);
            line = StripCitation(line);
            if (line.size() < 8)
                continue;
            uint64_t about = 0;
            for (const Known& k : people)
                if (Coa_FindName(line, k.name) != std::string::npos)
                {
                    about = k.guid;
                    break;
                }
            Memory_AddExperience(botGuid, about, line, importance, MEMORY_REFLECTION);
            ++reflections;
        }
    }

    bool ReflectionBudget()
    {
        const uint64_t now = NowSeconds();
        g_reflectionTimes.erase(std::remove_if(g_reflectionTimes.begin(), g_reflectionTimes.end(),
                                               [now](uint64_t t) { return t + 3600 < now; }),
                                g_reflectionTimes.end());
        if (g_ReflectionMaxPerHour && g_reflectionTimes.size() >= g_ReflectionMaxPerHour)
            return false;
        g_reflectionTimes.push_back(now);
        return true;
    }

    std::string WhoIs(Player* bot)
    {
        std::string who = std::string(bot->GetName()) + ", a level " + std::to_string(bot->GetLevel()) + " " +
                          Coa_RaceName(bot->getRace()) + " " + Coa_ClassName(bot->getClass()) + " (" +
                          (bot->getGender() == 0 ? "male" : "female") + "). Temperament: " +
                          GetPersonalityPromptAddition(GetBotPersonality(bot));
        return who;
    }

    // ---- rumours -------------------------------------------------------------

    std::unordered_map<uint64_t, uint64_t> g_lastGossip;     // pair -> when
    std::unordered_map<uint64_t, std::vector<std::string>> g_told;   // listener -> texts heard already
}

std::string Memory_RecallMarker(Player* bot, Player* about, const std::string& query)
{
    if (!g_MemoryEnable || !bot)
        return "";
    return std::string(MARKER_OPEN) + std::to_string(bot->GetGUID().GetRawValue()) + "|" +
           std::to_string(about ? about->GetGUID().GetRawValue() : 0) + "|" +
           Clean(about ? std::string(about->GetName()) : std::string()) + "|" + Clean(query.substr(0, 300)) +
           MARKER_CLOSE;
}

std::string Memory_ResolveRecall(const std::string& prompt)
{
    std::string out = prompt;
    for (size_t at; (at = out.find(MARKER_OPEN)) != std::string::npos;)
    {
        const size_t body = at + std::strlen(MARKER_OPEN);
        const size_t end = out.find(MARKER_CLOSE, body);
        if (end == std::string::npos)
        {
            out.erase(at);
            break;
        }
        const std::string fields = out.substr(body, end - body);
        size_t p1 = fields.find('|');
        size_t p2 = p1 == std::string::npos ? p1 : fields.find('|', p1 + 1);
        size_t p3 = p2 == std::string::npos ? p2 : fields.find('|', p2 + 1);
        std::string section;
        if (p3 != std::string::npos)
            section = Recall(std::strtoull(fields.substr(0, p1).c_str(), nullptr, 10),
                             std::strtoull(fields.substr(p1 + 1, p2 - p1 - 1).c_str(), nullptr, 10),
                             fields.substr(p2 + 1, p3 - p2 - 1), fields.substr(p3 + 1));
        out.replace(at, end + std::strlen(MARKER_CLOSE) - at, section);
    }
    return out;
}

void Memory_Update()
{
    if (!g_MemoryEnable || (!g_ReflectionEnable && !g_GoalsEnable))
        return;

    // Bots that have dealt with real people: an experience with one, or a bond.
    struct Due { uint64_t bot; bool reflect; bool goals; };
    std::vector<Due> due;
    const uint64_t now = NowSeconds();
    {
        std::lock_guard<std::mutex> lock(g_mutex);
        for (auto& [botGuid, state] : g_state)
        {
            if (due.size() >= 2)
                break;
            bool contact = false, hasGoal = false;
            for (const BotMemoryEntry& m : state.memories)
            {
                contact = contact || (m.aboutGuid != 0 && m.aboutGuid != botGuid && m.kind != MEMORY_RUMOUR);
                hasGoal = hasGoal || m.kind == MEMORY_GOAL;
            }
            if (!contact)
                continue;
            const bool reflect = g_ReflectionEnable && !state.reflecting &&
                                 state.importanceSinceReflection >= g_ReflectionThreshold &&
                                 state.lastReflection + uint64_t(g_ReflectionMinMinutes) * 60 <= now;
            const bool goals = g_GoalsEnable && !hasGoal && !state.goalsPending &&
                               g_lastGoalAttempt[botGuid] + 1800 <= now;
            if (reflect || goals)
                due.push_back({ botGuid, reflect, goals });
        }
    }

    for (const Due& d : due)
    {
        Player* bot = ObjectAccessor::FindConnectedPlayer(ObjectGuid(d.bot));
        if (!bot || !ReflectionBudget())
            continue;

        // The people this bot knows, so a reflection about one of them is filed under them.
        std::vector<Known> people;
        std::vector<BotMemoryEntry> memories;
        {
            std::lock_guard<std::mutex> lock(g_mutex);
            OllamaMemoryState& state = g_state[d.bot];
            for (const auto& [guid, rel] : state.relationships)
                if (!rel.otherName.empty())
                    people.push_back({ guid, rel.otherName });
            memories = state.memories;
            if (d.reflect)
            {
                state.reflecting = true;
                state.importanceSinceReflection = 0;
                state.lastReflection = now;
            }
            if (d.goals)
                state.goalsPending = true;
        }
        g_lastGoalAttempt[d.bot] = now;
        // People named in the memories but without a relationship sentence yet.
        for (const BotMemoryEntry& m : memories)
            if (m.aboutGuid && m.aboutGuid != d.bot &&
                std::none_of(people.begin(), people.end(), [&](const Known& k) { return k.guid == m.aboutGuid; }))
                if (Player* p = ObjectAccessor::FindPlayer(ObjectGuid(m.aboutGuid)))
                    people.push_back({ m.aboutGuid, p->GetName() });

        const std::string who = WhoIs(bot);
        const uint64_t botGuid = d.bot;
        const bool reflect = d.reflect, goals = d.goals;

        // The 40 most recent memories, numbered, as the paper hands the model the latest records.
        std::sort(memories.begin(), memories.end(),
                  [](const BotMemoryEntry& a, const BotMemoryEntry& b) { return a.createdAt > b.createdAt; });
        if (memories.size() > 40)
            memories.resize(40);
        std::string statements, currentGoals;
        int n = 0;
        for (const BotMemoryEntry& m : memories)
        {
            if (m.kind == MEMORY_GOAL)
                currentGoals += " - " + m.text + "\n";
            else
                statements += std::to_string(++n) + ". " + m.text + "\n";
        }
        std::vector<std::string> recent;
        for (const BotMemoryEntry& m : memories)
            if (m.kind != MEMORY_GOAL)
                recent.push_back(m.text);

        OllamaDispatch_SubmitJob([botGuid, who, statements, currentGoals, recent, people, reflect, goals]()
        {
            std::string answer;
            if (reflect && !statements.empty())
            {
                // Step 1 (the paper): what is worth thinking about?
                OllamaApiResult q = QueryOllama(
                    "Statements about " + who + "\n" + statements +
                    "Given only the information above, what are the 3 most salient high-level questions we can "
                    "answer about the people in the statements? One per line, nothing else.",
                    OllamaRequestKind::Sentiment);
                std::string questions = q.ok ? q.text : std::string();

                // Step 2: the memories that bear on those questions, and the insights they support.
                std::string evidence = statements;
                if (!questions.empty())
                {
                    std::vector<BotMemoryEntry> pool;
                    for (const std::string& t : recent)
                    {
                        BotMemoryEntry e;
                        e.text = t;
                        e.importance = 5;
                        e.createdAt = e.lastAccess = NowSeconds();
                        pool.push_back(std::move(e));
                    }
                    std::vector<const BotMemoryEntry*> ptrs;
                    for (const BotMemoryEntry& e : pool)
                        ptrs.push_back(&e);
                    std::vector<Scored> ranked = Rank(ptrs, questions);
                    evidence.clear();
                    int k = 0;
                    for (const Scored& s : ranked)
                    {
                        if (k >= 20)
                            break;
                        evidence += std::to_string(++k) + ". " + s.memory->text + "\n";
                    }
                }

                std::string prompt = "Statements about " + who + "\n" + evidence;
                if (!questions.empty())
                    prompt += "Questions worth thinking about:\n" + questions + "\n";
                prompt += "What 3 high-level insights can you infer from the above statements, about yourself, the "
                          "people you know and what you want? Write each as a short sentence spoken to the person the "
                          "statements are about ('You ...'), starting with its importance from 1 to 10 in brackets, "
                          "for example: [7] You trust Chris with your life (because of 1, 4).";
                if (goals || !currentGoals.empty())
                    prompt += " Then, on one more line starting with 'GOAL:', a personal goal or dream for the coming "
                              "weeks that follows from all this" +
                              std::string(currentGoals.empty() ? "" : " (your goals so far:\n" + currentGoals + "keep "
                              "one of them if it still fits, or say how it has changed)") + ".";
                OllamaApiResult r = QueryOllama(prompt, OllamaRequestKind::Sentiment);
                if (r.ok)
                    answer = r.text;
            }
            else if (goals)
            {
                OllamaApiResult r = QueryOllama(
                    "You are " + who + " You live in the world of Azeroth" +
                    std::string(statements.empty() ? "." : ", and this is what you went through lately:\n" + statements) +
                    "\nGive two personal goals or dreams for the coming weeks or for your life that fit who you are "
                    "and your people - something to strive for, to find, to prove, to save for or to set right. Each on "
                    "its own line, starting with 'GOAL:', short, spoken to yourself ('You want to ...').",
                    OllamaRequestKind::Sentiment);
                if (r.ok)
                    answer = r.text;
            }

            if (!answer.empty())
                StoreReflections(botGuid, Lines(answer), people);

            std::lock_guard<std::mutex> lock(g_mutex);
            OllamaMemoryState& state = g_state[botGuid];
            state.reflecting = false;
            state.goalsPending = false;
            if (g_DebugEnabled)
                LOG_INFO("module.ollamachat", "[Ollama Chat] Bot {} reflected{}: {}", botGuid,
                         goals ? " and set goals" : "", answer);
        });
    }
}

void Memory_ShareRumour(Player* teller, Player* listener)
{
    if (!g_MemoryEnable || !g_GossipEnable || !teller || !listener || teller == listener)
        return;
    if (urand(1, 100) > g_GossipChance)
        return;

    const uint64_t tellerGuid = teller->GetGUID().GetRawValue();
    const uint64_t listenerGuid = listener->GetGUID().GetRawValue();
    const uint64_t pair = tellerGuid * 0x9E3779B97F4A7C15ull ^ listenerGuid;
    const uint64_t now = NowSeconds();
    if (g_lastGossip[pair] + 1800 > now)
        return;

    // Something the teller went through with, or thinks of, a real person - worth retelling.
    BotMemoryEntry chosen;
    {
        std::lock_guard<std::mutex> lock(g_mutex);
        auto it = g_state.find(tellerGuid);
        if (it == g_state.end())
            return;
        std::vector<const BotMemoryEntry*> worth;
        const std::vector<std::string>& told = g_told[listenerGuid];
        for (const BotMemoryEntry& m : it->second.memories)
            if ((m.kind == MEMORY_EXPERIENCE || m.kind == MEMORY_REFLECTION) && m.importance >= 4 && m.aboutGuid &&
                m.aboutGuid != tellerGuid && m.aboutGuid != listenerGuid &&
                std::find(told.begin(), told.end(), m.text) == told.end())
                worth.push_back(&m);
        if (worth.empty())
            return;
        chosen = *worth[urand(0, uint32(worth.size() - 1))];
    }
    Player* subject = ObjectAccessor::FindPlayer(ObjectGuid(chosen.aboutGuid));
    if (subject && OllamaIsBotPlayer(subject))
        return;     // a rumour is about a real person
    g_lastGossip[pair] = now;
    g_told[listenerGuid].push_back(chosen.text);
    if (g_told[listenerGuid].size() > 50)
        g_told[listenerGuid].erase(g_told[listenerGuid].begin());

    const std::string tellerName = teller->GetName();
    const std::string listenerName = listener->GetName();
    const std::string prompt =
        "This is something " + tellerName + " remembers, written to " + tellerName + " ('you' means " + tellerName +
        "): \"" + chosen.text + "\"\n" + tellerName + " told it to " + listenerName + ". Write in one short sentence "
        "what " + listenerName + " heard, spoken to " + listenerName + ", beginning with \"" + tellerName +
        " told you\". Nothing else.";
    const uint64_t about = chosen.aboutGuid;
    const uint8_t importance = uint8_t(std::max(2, int(chosen.importance) - 2));
    OllamaDispatch_SubmitJob([prompt, listenerGuid, about, importance, tellerName]()
    {
        OllamaApiResult r = QueryOllama(prompt, OllamaRequestKind::Sentiment);
        if (!r.ok)
            return;
        std::vector<std::string> lines = Lines(r.text);
        if (lines.empty() || lines[0].find(tellerName) == std::string::npos)
            return;
        Memory_AddExperience(listenerGuid, about, lines[0], importance, MEMORY_RUMOUR);
    });
}
