#include "mod-ollama-chat_bonds.h"
#include "CoaChatBridge.h"
#include "mod-ollama-chat_api.h"
#include "mod-ollama-chat_coa.h"
#include "mod-ollama-chat_config.h"
#include "mod-ollama-chat_dispatch.h"
#include "mod-ollama-chat_governor.h"
#include "mod-ollama-chat_handler.h"
#include "mod-ollama-chat_memory.h"
#include "mod-ollama-chat_personality.h"
#include "mod-ollama-chat_roleplay.h"
#include "mod-ollama-chat_world.h"
#include "mod-ollama-chat-utilities.h"

#include "Creature.h"
#include "DatabaseEnv.h"
#include "Group.h"
#include "Log.h"
#include "ObjectAccessor.h"
#include "Player.h"
#include "QuestDef.h"

#include "PlayerbotAI.h"
#include "PlayerbotMgr.h"

#include <algorithm>
#include <ctime>
#include <mutex>
#include <unordered_map>
#include <vector>

namespace
{
    struct Bond
    {
        std::string otherName;
        int32_t     affinity   = 0;     // -100 enemy .. 100 closest friend
        int32_t     romance    = 0;     // 0 .. 100
        uint32_t    talks      = 0;
        uint32_t    shared     = 0;
        std::string lastTopic;
        uint64_t    lastSeen   = 0;     // unix seconds of the last exchange
        uint64_t    topicFrom  = 0;     // lastSeen of the conversation lastTopic sums up
        bool        summarizing = false;
        bool        dirty       = false;
    };

    std::mutex g_bondMutex;
    std::unordered_map<uint64_t, std::unordered_map<uint64_t, Bond>> g_bonds;   // bot -> person -> bond

    uint64_t Now()
    {
        return static_cast<uint64_t>(time(nullptr));
    }

    std::string Escape(std::string v)
    {
        CharacterDatabase.EscapeString(v);
        return v;
    }

    // The step of a bond, as the prompt and the roleplay add-on both name it.
    std::string StandingOf(Bond const& b)
    {
        if (b.talks + b.shared == 0)                              return "stranger";
        if (b.affinity <= -60)                                    return "enemy";
        if (b.affinity <= -25)                                    return "dislike";
        if (b.affinity < -5)                                      return "distrust";
        if (g_BondRomance && b.romance >= 60 && b.affinity >= 50) return "beloved";
        if (g_BondRomance && b.romance >= 30 && b.affinity >= 30) return "drawn";
        if (b.affinity >= 70)                                     return "close";
        if (b.affinity >= 40)                                     return "friend";
        if (b.affinity >= 15)                                     return "like";
        return "know";
    }

    BondStanding ToStanding(Bond const& b)
    {
        BondStanding s;
        s.standing  = StandingOf(b);
        s.affinity  = b.affinity;
        s.romance   = g_BondRomance ? b.romance : 0;
        s.talks     = b.talks;
        s.shared    = b.shared;
        s.lastTopic = b.lastTopic;
        return s;
    }

    bool IsBot(Player* p)
    {
        return p && OllamaIsBotPlayer(p);
    }

    std::string Ago(uint64_t at)
    {
        const uint64_t now = Now();
        const uint64_t d = now > at ? now - at : 0;
        if (d < 3600)            return "less than an hour ago";
        if (d < 6 * 3600)        return "a few hours ago";
        if (d < 24 * 3600)       return "earlier today";
        if (d < 2 * 24 * 3600)   return "yesterday";
        if (d < 7 * 24 * 3600)   return "a few days ago";
        if (d < 30 * 24 * 3600)  return "some weeks ago";
        return "a long time ago";
    }

    std::string Times(uint32_t n)
    {
        if (n <= 1)  return "once";
        if (n <= 3)  return "a few times";
        if (n <= 10) return "several times";
        if (n <= 40) return "many times";
        return "countless times";
    }

    // The members of `player`'s group near them (the player included), split into bots and people.
    void GroupAround(Player* player, std::vector<Player*>& bots, std::vector<Player*>& people)
    {
        auto add = [&](Player* p)
        {
            if (!p || !p->IsInWorld())
                return;
            if (p != player && (p->GetMapId() != player->GetMapId() || p->GetDistance(player) > 80.0f))
                return;
            (IsBot(p) ? bots : people).push_back(p);
        };
        if (Group* group = player->GetGroup())
        {
            for (GroupReference* ref = group->GetFirstMember(); ref; ref = ref->next())
                add(ref->GetSource());
        }
        else
            add(player);
    }

    std::string ZoneName(Player* p)
    {
        if (PlayerbotAI* ai = PlayerbotsMgr::instance().GetPlayerbotAI(p))
            if (AreaTableEntry const* zone = ai->GetCurrentZone())
                return PlayerbotAI::GetLocalizedAreaName(zone);
        return "";
    }

    std::string In(Player* p)
    {
        std::string zone = ZoneName(p);
        return zone.empty() ? std::string() : " in " + zone;
    }

    // ---- the bridge's voice ------------------------------------------------

    bool Voice(Player* bot, Player* to, std::string const& line, uint8_t channel, uint8_t mode,
               std::string const& context)
    {
        if (!g_Enable || !g_VoiceEnable || !bot || !bot->IsInWorld() || line.empty() || !IsBot(bot))
            return false;
        if (channel == CoaChatBridge::CHANNEL_WHISPER && !to)
            return false;

        PlayerbotAI* botAI = PlayerbotsMgr::instance().GetPlayerbotAI(bot);
        if (!botAI || !botAI->GetChatHelper())
            return false;

        std::string marked = line;
        std::vector<std::string> links = Coa_TakeLinks(marked);

        const std::string name = bot->GetName();
        std::string prompt = "You are " + name + ", a level " + std::to_string(bot->GetLevel()) + " " +
            Coa_RaceName(bot->getRace()) + " " + botAI->GetChatHelper()->FormatClass(bot->getClass()) + ". " +
            "Your temperament: " + GetPersonalityPromptAddition(GetBotPersonality(bot)) + " " +
            OllamaDescribeSituation(bot, to);
        if (to)
        {
            prompt += Bond_BuildPromptSection(bot, to);
            prompt += Memory_RecallMarker(bot, to, line);
        }
        prompt += Roleplay_BuildVoicePrompt(bot);
        if (!context.empty())
            prompt += " " + context;

        if (mode == CoaChatBridge::MODE_EXACT)
        {
            prompt += std::string(" Say the following") + (to ? " to " + std::string(to->GetName()) : "") +
                " in your own words, the way you would say it, in one or two short sentences. Keep its meaning, "
                "and keep every number, price, amount, item name and place exactly as it is";
            if (!links.empty())
                prompt += "; keep the markers [1], [2] and so on exactly as they are, they stand for items";
            prompt += ": \"" + marked + "\"";
        }
        else
            prompt += std::string(" Say something") + (to ? " to " + std::string(to->GetName()) : " to the people around you") +
                " in the spirit of this line, but in your own words and fitting the moment: \"" + marked + "\"";

        OllamaChatRequest request;
        request.botGuid   = bot->GetGUID().GetRawValue();
        request.targetGuid = to ? to->GetGUID().GetRawValue() : 0;
        request.source    = channel == CoaChatBridge::CHANNEL_WHISPER ? SRC_WHISPER_LOCAL
                          : channel == CoaChatBridge::CHANNEL_YELL    ? SRC_YELL_LOCAL
                                                                      : SRC_SAY_LOCAL;
        request.scopeKey  = Governor_MakeScopeKey(ChatChannelSourceLocalStr[request.source], 0, std::string(), 0,
                                                  bot->GetZoneId());
        request.directAddress    = true;
        request.prompt           = std::move(prompt);
        request.botName          = name;
        request.kind             = OllamaRequestKind::RoleplayReply;
        request.triggerBotReplies = false;
        request.voice            = true;
        request.voiceExact       = mode == CoaChatBridge::MODE_EXACT;
        request.voiceOriginal    = line;
        request.voiceLinks       = std::move(links);
        return OllamaDispatch_Submit(std::move(request));
    }

    void Experience(Player* bot, Player* with, std::string const& what, uint8_t importance, int8_t bond)
    {
        if (!bot || what.empty() || !IsBot(bot))
            return;
        const uint64_t about = with ? with->GetGUID().GetRawValue() : bot->GetGUID().GetRawValue();
        Memory_AddExperience(bot->GetGUID().GetRawValue(), about, what, importance);
        if (with && !IsBot(with))
            Bond_NoteShared(bot, with, bond);
    }

    // Not the same kill twice from every member of a group within a few minutes.
    std::unordered_map<uint64_t, time_t> g_recent;
    bool Fresh(uint64_t a, uint64_t b, uint32_t c, time_t window)
    {
        const time_t now = time(nullptr);
        if (g_recent.size() > 4096)
            for (auto it = g_recent.begin(); it != g_recent.end();)
                it = it->second + 3600 < now ? g_recent.erase(it) : std::next(it);
        const uint64_t key = (a * 0x9E3779B97F4A7C15ull) ^ (b * 0xC2B2AE3D27D4EB4Full) ^ c;
        auto it = g_recent.find(key);
        if (it != g_recent.end() && it->second + window > now)
            return false;
        g_recent[key] = now;
        return true;
    }
}

// ---------------------------------------------------------------------------

void Bond_Load()
{
    std::lock_guard<std::mutex> lock(g_bondMutex);
    g_bonds.clear();
    QueryResult result = CharacterDatabase.Query(
        "SELECT bot_guid, other_guid, other_name, affinity, romance, talks, shared, last_topic, last_seen "
        "FROM mod_ollama_chat_bonds");
    if (!result)
        return;
    uint32_t loaded = 0;
    do
    {
        Field* f = result->Fetch();
        Bond& b = g_bonds[f[0].Get<uint64>()][f[1].Get<uint64>()];
        b.otherName = f[2].Get<std::string>();
        b.affinity  = f[3].Get<int16>();
        b.romance   = f[4].Get<int16>();
        b.talks     = f[5].Get<uint32>();
        b.shared    = f[6].Get<uint32>();
        b.lastTopic = f[7].Get<std::string>();
        b.lastSeen  = f[8].Get<uint64>();
        b.topicFrom = b.lastSeen;
        ++loaded;
    } while (result->NextRow());
    LOG_INFO("module.ollamachat", "[Ollama Chat] Loaded {} bonds between bots and people.", loaded);
}

void Bond_SaveAll()
{
    std::lock_guard<std::mutex> lock(g_bondMutex);
    CharacterDatabaseTransaction trans;
    for (auto& [botGuid, people] : g_bonds)
        for (auto& [otherGuid, b] : people)
        {
            if (!b.dirty)
                continue;
            if (!trans)
                trans = CharacterDatabase.BeginTransaction();
            trans->Append(SafeFormat(
                "INSERT INTO mod_ollama_chat_bonds (bot_guid, other_guid, other_name, affinity, romance, talks, shared, "
                "last_topic, last_seen) VALUES ({}, {}, '{}', {}, {}, {}, {}, '{}', {}) ON DUPLICATE KEY UPDATE "
                "other_name = VALUES(other_name), affinity = VALUES(affinity), romance = VALUES(romance), "
                "talks = VALUES(talks), shared = VALUES(shared), last_topic = VALUES(last_topic), "
                "last_seen = VALUES(last_seen)",
                botGuid, otherGuid, Escape(b.otherName), b.affinity, b.romance, b.talks, b.shared,
                Escape(b.lastTopic), b.lastSeen));
            b.dirty = false;
        }
    if (trans)
        CharacterDatabase.CommitTransaction(trans);
}

void Bond_Forget(uint64_t guid)
{
    std::lock_guard<std::mutex> lock(g_bondMutex);
    g_bonds.erase(guid);
    for (auto& [bot, people] : g_bonds)
        people.erase(guid);
}

void Bond_NoteTalk(Player* bot, Player* other)
{
    if (!g_BondEnable || !bot || !other || IsBot(other))
        return;
    std::lock_guard<std::mutex> lock(g_bondMutex);
    Bond& b = g_bonds[bot->GetGUID().GetRawValue()][other->GetGUID().GetRawValue()];
    b.otherName = other->GetName();
    ++b.talks;
    b.lastSeen = Now();
    b.dirty = true;
}

void Bond_ApplyTone(uint64_t botGuid, uint64_t otherGuid, BondTone tone)
{
    if (!g_BondEnable || tone == BondTone::Neutral)
        return;
    std::lock_guard<std::mutex> lock(g_bondMutex);
    auto botIt = g_bonds.find(botGuid);
    if (botIt == g_bonds.end())
        return;
    auto it = botIt->second.find(otherGuid);
    if (it == botIt->second.end())
        return;
    Bond& b = it->second;
    switch (tone)
    {
        case BondTone::Friendly:
            b.affinity += 2;
            break;
        case BondTone::Romantic:
            b.affinity += 2;
            // Flirting from someone the bot hardly knows flatters a little; from a friend it can become more.
            if (g_BondRomance)
                b.romance += b.affinity >= 40 ? 6 : (b.romance < 25 ? 2 : 0);
            break;
        case BondTone::Hostile:
            b.affinity -= 6;
            b.romance -= 10;
            break;
        default:
            break;
    }
    b.affinity = std::clamp(b.affinity, -100, 100);
    b.romance  = std::clamp(b.romance, 0, 100);
    b.dirty = true;
}

void Bond_NoteShared(Player* bot, Player* other, int8_t change)
{
    if (!g_BondEnable || !bot || !other || IsBot(other))
        return;
    std::lock_guard<std::mutex> lock(g_bondMutex);
    Bond& b = g_bonds[bot->GetGUID().GetRawValue()][other->GetGUID().GetRawValue()];
    b.otherName = other->GetName();
    ++b.shared;
    b.affinity = std::clamp<int32_t>(b.affinity + change, -100, 100);
    b.dirty = true;
}

BondStanding Bond_Standing(uint64_t botGuid, uint64_t otherGuid)
{
    std::lock_guard<std::mutex> lock(g_bondMutex);
    auto botIt = g_bonds.find(botGuid);
    if (botIt == g_bonds.end())
        return BondStanding();
    auto it = botIt->second.find(otherGuid);
    return it == botIt->second.end() ? BondStanding() : ToStanding(it->second);
}

std::vector<std::pair<uint64_t, BondStanding>> Bond_ListFor(uint64_t otherGuid)
{
    std::vector<std::pair<uint64_t, BondStanding>> out;
    std::lock_guard<std::mutex> lock(g_bondMutex);
    for (auto const& [bot, people] : g_bonds)
    {
        auto it = people.find(otherGuid);
        if (it != people.end() && it->second.talks + it->second.shared > 0)
            out.emplace_back(bot, ToStanding(it->second));
    }
    return out;
}

std::string Bond_BuildPromptSection(Player* bot, Player* other)
{
    if (!g_BondEnable || !bot || !other || IsBot(other))
        return "";

    Bond b;
    {
        std::lock_guard<std::mutex> lock(g_bondMutex);
        auto botIt = g_bonds.find(bot->GetGUID().GetRawValue());
        if (botIt != g_bonds.end())
        {
            auto it = botIt->second.find(other->GetGUID().GetRawValue());
            if (it != botIt->second.end())
                b = it->second;
        }
    }

    const std::string who = other->GetName();
    std::string out = " Who " + who + " is to you: ";
    const uint32_t known = b.talks + b.shared;
    const std::string standing = StandingOf(b);

    if (standing == "stranger")
        out += "a stranger; you have never met. Be as open or as wary as your temperament makes you.";
    else if (standing == "enemy")
        out += "your enemy. You are openly hostile, may insult or threaten them, and do them no favours.";
    else if (standing == "dislike")
        out += "someone you dislike. You are cold, curt and sarcastic with them.";
    else if (standing == "distrust")
        out += "someone you do not trust. You are guarded and short with them.";
    else if (standing == "beloved")
        out += "your beloved. You are tender, affectionate and a little protective, with private jokes between "
               "you; keep it warm and tasteful, never explicit.";
    else if (standing == "drawn")
        out += "someone you are drawn to. You enjoy their attention and flirt back, shyly or boldly as your "
               "temperament allows; keep it tasteful.";
    else if (standing == "close")
        out += "one of your closest friends. You trust them completely, tease them and would fight for them.";
    else if (standing == "friend")
        out += "your friend. You are warm and at ease with them, joke and speak openly.";
    else if (standing == "like")
        out += "someone you like. You are glad to see them.";
    else
        out += "someone you know a little.";

    if (known)
    {
        out += " You have talked " + Times(b.talks);
        if (b.shared)
            out += " and been through things together " + Times(b.shared);
        out += ".";
    }

    // A conversation that went quiet: when it was and what it was about.
    if (b.lastSeen && b.lastSeen + 15 * 60 < Now())
    {
        out += " You last spoke " + Ago(b.lastSeen);
        if (!b.lastTopic.empty())
            out += "; " + b.lastTopic;
        else
            out += ".";
        if (b.lastSeen + 2 * 24 * 3600 < Now())
            out += " It has been a while; you may remark on it.";
    }
    return out;
}

void Bond_Update()
{
    if (!g_BondEnable)
        return;

    // Conversations quiet for three minutes and not yet summed up. At most two per pass: this is background work.
    struct Pending { uint64_t bot; uint64_t other; uint64_t seen; std::string botName; std::string otherName; };
    std::vector<Pending> pending;
    const uint64_t now = Now();
    {
        std::lock_guard<std::mutex> lock(g_bondMutex);
        for (auto& [botGuid, people] : g_bonds)
            for (auto& [otherGuid, b] : people)
            {
                if (pending.size() >= 2)
                    break;
                if (b.summarizing || b.lastSeen == 0 || b.topicFrom == b.lastSeen || b.lastSeen + 180 > now)
                    continue;
                Player* bot = ObjectAccessor::FindConnectedPlayer(ObjectGuid(botGuid));
                if (!bot)
                    continue;
                b.summarizing = true;
                pending.push_back({ botGuid, otherGuid, b.lastSeen, bot->GetName(), b.otherName });
            }
    }

    for (const Pending& p : pending)
    {
        std::string lines;
        {
            std::lock_guard<std::mutex> lock(g_ConversationHistoryMutex);
            auto botIt = g_BotConversationHistory.find(p.bot);
            if (botIt != g_BotConversationHistory.end())
            {
                auto it = botIt->second.find(p.other);
                if (it != botIt->second.end())
                {
                    size_t skip = it->second.size() > 6 ? it->second.size() - 6 : 0;
                    for (const auto& e : it->second)
                    {
                        if (skip) { --skip; continue; }
                        if (!e.playerMessage.empty() && e.playerMessage[0] != '(')
                            lines += p.otherName + ": " + e.playerMessage + "\n";
                        if (!e.botReply.empty())
                            lines += p.botName + ": " + e.botReply + "\n";
                    }
                }
            }
        }

        auto done = [p](std::string topic)
        {
            std::lock_guard<std::mutex> lock(g_bondMutex);
            Bond& b = g_bonds[p.bot][p.other];
            b.summarizing = false;
            b.topicFrom = p.seen;
            if (!topic.empty())
            {
                b.lastTopic = topic;
                b.dirty = true;
            }
        };

        if (lines.empty())
        {
            done(std::string());
            continue;
        }

        const std::string prompt =
            "Here is a conversation between " + p.botName + " and " + p.otherName + ":\n" + lines +
            "In one short sentence, say what they talked about and how it ended, speaking to " + p.botName +
            ". Begin with \"you talked about\". Nothing else.";

        if (!OllamaDispatch_SubmitJob([prompt, done]()
            {
                OllamaApiResult api = QueryOllama(prompt, OllamaRequestKind::Sentiment);
                std::string topic;
                if (api.ok)
                {
                    topic = api.text;
                    for (char& c : topic)
                        if (c == '\n' || c == '\r' || c == '"')
                            c = ' ';
                    size_t a = topic.find_first_not_of(' ');
                    topic = a == std::string::npos ? std::string() : topic.substr(a);
                    if (topic.size() > 220)
                    {
                        topic.resize(220);
                        size_t cut = topic.find_last_of(' ');
                        if (cut != std::string::npos)
                            topic.resize(cut);
                    }
                    if (!topic.empty() && topic.back() != '.')
                        topic += ".";
                    if (!topic.empty())
                        topic[0] = static_cast<char>(std::tolower(static_cast<unsigned char>(topic[0])));
                }
                done(topic);
            }))
        {
            std::lock_guard<std::mutex> lock(g_bondMutex);
            g_bonds[p.bot][p.other].summarizing = false;   // try again next pass
        }
    }
}

std::string Bond_PersonaSection(Player* bot)
{
    std::string out;
    for (auto const& fn : CoaChatBridge::Personas())
    {
        if (!fn)
            continue;
        std::string s = fn(bot);
        if (!s.empty())
            out += (out.empty() ? "" : " ") + s;
    }
    return out;
}

void Bond_RegisterBridge()
{
    static bool registered = false;
    if (registered)
        return;
    registered = true;
    CoaChatBridge::Voice() = Voice;
    CoaChatBridge::Experience() = Experience;
}

// ---------------------------------------------------------------------------
// Experiences from the game
// ---------------------------------------------------------------------------

void Experience_OnCreatureKill(Player* killer, Creature* victim)
{
    if (!g_MemoryEnable || !killer || !victim || !victim->GetCreatureTemplate())
        return;

    // Only what is worth telling: elites, rares and bosses.
    const uint32_t rank = victim->GetCreatureTemplate()->rank;
    const bool boss = victim->isWorldBoss() || victim->IsDungeonBoss();
    if (rank == CREATURE_ELITE_NORMAL && !boss)
        return;

    std::vector<Player*> bots, people;
    GroupAround(killer, bots, people);
    const std::string where = In(killer);
    const std::string foe = victim->GetName();
    const uint8_t weight = boss ? 7 : (rank == CREATURE_ELITE_RAREELITE || rank == CREATURE_ELITE_RARE) ? 5 : 3;

    for (Player* bot : bots)
    {
        if (!Fresh(bot->GetGUID().GetRawValue(), 0, victim->GetEntry(), 600))
            continue;
        if (people.empty())
            Experience(bot, nullptr, "You defeated " + foe + where + ".", uint8_t(weight > 3 ? weight - 2 : 2), 0);
        for (Player* person : people)
            Experience(bot, person, "You and " + std::string(person->GetName()) + " fought and defeated " + foe +
                       where + ".", weight, int8_t(boss ? 4 : 2));
    }
}

void Experience_OnQuestComplete(Player* player, Quest const* quest)
{
    if (!g_MemoryEnable || !player || !quest)
        return;

    std::vector<Player*> bots, people;
    GroupAround(player, bots, people);
    const std::string title = quest->GetTitle();
    const std::string where = In(player);

    if (IsBot(player))
    {
        if (!Fresh(player->GetGUID().GetRawValue(), 1, quest->GetQuestId(), 3600))
            return;
        if (people.empty())
            Experience(player, nullptr, "You finished the task \"" + title + "\"" + where + ".", 2, 0);
        for (Player* person : people)
            Experience(player, person, "You and " + std::string(person->GetName()) + " finished the task \"" + title +
                       "\"" + where + " together.", 5, 3);
    }
    else
    {
        // A person finishing a task with bots at their side: the bots were there.
        for (Player* bot : bots)
            if (Fresh(bot->GetGUID().GetRawValue(), player->GetGUID().GetRawValue(), quest->GetQuestId(), 3600))
                Experience(bot, player, "You helped " + std::string(player->GetName()) + " with the task \"" + title +
                           "\"" + where + ".", 4, 2);
    }
}

void Experience_OnDeath(Player* player)
{
    if (!g_MemoryEnable || !player)
        return;

    std::vector<Player*> bots, people;
    GroupAround(player, bots, people);
    const std::string where = In(player);

    if (IsBot(player))
    {
        if (!Fresh(player->GetGUID().GetRawValue(), 2, 0, 300))
            return;
        if (people.empty())
            Experience(player, nullptr, "You fell in battle" + where + " and had to make your way back.", 3, 0);
        for (Player* person : people)
            Experience(player, person, "You fell in battle" + where + " while travelling with " +
                       std::string(person->GetName()) + ".", 4, 0);
    }
    else
    {
        for (Player* bot : bots)
            if (bot != player && Fresh(bot->GetGUID().GetRawValue(), player->GetGUID().GetRawValue(), 3, 300))
                Experience(bot, player, "You saw " + std::string(player->GetName()) + " fall in battle" + where + ".", 4, 1);
    }
}

void Experience_OnDuelEnd(Player* winner, Player* loser)
{
    if (!g_MemoryEnable || !winner || !loser)
        return;
    if (IsBot(winner) && !IsBot(loser))
        Experience(winner, loser, "You beat " + std::string(loser->GetName()) + " in a duel" + In(winner) + ".", 5, 1);
    if (IsBot(loser) && !IsBot(winner))
        Experience(loser, winner, "You lost a duel to " + std::string(winner->GetName()) + In(loser) + ".", 5, 1);
}
