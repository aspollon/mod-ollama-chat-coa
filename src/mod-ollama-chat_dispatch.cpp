#include "mod-ollama-chat_dispatch.h"
#include "mod-ollama-chat_voice.h"
#include "mod-ollama-chat_api.h"
#include "mod-ollama-chat_config.h"
#include "mod-ollama-chat_expression.h"
#include "mod-ollama-chat_governor.h"
#include "mod-ollama-chat_memory.h"
#include "mod-ollama-chat_response.h"
#include "mod-ollama-chat_roleplay.h"
#include "mod-ollama-chat_sentiment.h"
#include "mod-ollama-chat-utilities.h"
#include "mod-ollama-chat_world.h"
#include "mod-ollama-chat_coa.h"
#include "mod-ollama-chat_bonds.h"
#include "mod-ollama-chat_presence.h"

#include "CellImpl.h"
#include "Channel.h"
#include "ChannelMgr.h"
#include "GridNotifiers.h"
#include "GridNotifiersImpl.h"
#include "Group.h"
#include "Guild.h"
#include "GuildMgr.h"
#include "Log.h"
#include "ObjectAccessor.h"
#include "Player.h"

#include "PlayerbotAI.h"
#include "PlayerbotMgr.h"

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <deque>
#include <list>
#include <map>
#include <unordered_map>
#include <mutex>
#include <thread>
#include <vector>

namespace
{
    using Clock = std::chrono::steady_clock;

    enum class TaskType : uint8_t { ChatReply, Sentiment, Condense, Relationship, Job };

    struct Task
    {
        TaskType          type = TaskType::ChatReply;
        OllamaChatRequest request;

        // Sentiment-only payload.
        uint64_t    sentimentBotGuid    = 0;
        uint64_t    sentimentPlayerGuid = 0;
        std::string sentimentMessage;
        std::string sentimentPrompt;

        // Background job payload.
        std::function<void()> job;

        // Memory / relationship payload.
        uint64_t    memoryBotGuid   = 0;
        uint64_t    memoryOtherGuid = 0;
        std::string memoryOtherName;
        std::string memoryPrompt;
    };

    struct Completion
    {
        OllamaChatRequest request;
        std::string       text;
        std::string       action;   // an emote line in the third person, sent before the words
        uint32_t          emoteId = 0;
        Clock::time_point deliverAt;
    };

    // --- shared state -----------------------------------------------------

    std::mutex              g_queueMutex;
    std::condition_variable g_queueCv;
    std::deque<Task>        g_queue;
    bool                    g_running = false;

    std::mutex             g_doneMutex;
    std::deque<Completion> g_done;

    std::vector<std::thread> g_workers;

    std::atomic<uint32_t> g_inFlight{ 0 };
    std::atomic<uint64_t> g_totalSubmitted{ 0 };
    std::atomic<uint64_t> g_totalDelivered{ 0 };
    std::atomic<uint64_t> g_droppedQueueFull{ 0 };
    std::atomic<uint64_t> g_droppedEmpty{ 0 };
    std::atomic<uint64_t> g_droppedGovernor{ 0 };
    std::atomic<uint64_t> g_totalFailed{ 0 };

    std::mutex  g_errorMutex;
    std::string g_lastError;

    void RecordError(const std::string& what)
    {
        ++g_totalFailed;
        std::lock_guard<std::mutex> lock(g_errorMutex);
        g_lastError = what;
    }

    // --- worker -----------------------------------------------------------

    // The last line each bot said to someone directly (bot guid -> text), for the repetition check below.
    std::mutex g_lastDirectMutex;
    std::unordered_map<uint64_t, std::string> g_lastDirect;

    // ---------------------------------------------------------------------
    // Conversation contexts
    //
    // Ollama returns the tokenized prompt plus the reply from /api/generate.
    // Sending that array back with only the next line continues the same
    // sequence: the character sheet, lore, voice and history are not repeated
    // and, when the runner still holds the matching prefix, not read again.
    //
    // The context is per (bot, other), not per bot. Another bot speaking to
    // the player, a bot-to-bot conversation or ambient chatter therefore
    // cannot mix two conversations: each carries its own token array, and a
    // request that finds no context simply sends the full prompt.
    // ---------------------------------------------------------------------
    struct ConversationContext
    {
        std::vector<int> tokens;
        std::string      model;
        std::string      url;
        std::string      systemPrompt;
        uint32_t         numCtx       = 0;
        uint32_t         generation   = 0;
        uint64_t         sessionHash  = 0;
        bool             inFlight     = false;
        Clock::time_point lastUsed{};
        Clock::time_point inFlightSince{};
    };

    std::mutex g_conversationMutex;
    std::map<std::pair<uint64_t, uint64_t>, ConversationContext> g_conversations;

    constexpr size_t kMaxConversationContexts = 512;
    constexpr auto   kConversationTtl         = std::chrono::minutes(30);
    constexpr auto   kConversationInFlightTtl = std::chrono::minutes(5);

    bool TakeConversation(uint64_t botGuid, uint64_t otherGuid, uint64_t sessionHash,
                          size_t deltaChars, std::vector<int>& out)
    {
        if (!g_ReuseConversationContext || botGuid == 0 || otherGuid == 0)
            return false;

        const OllamaEndpointSettings cfg = OllamaConfig_Snapshot();
        if (cfg.provider != OllamaProvider::Ollama)
            return false;

        std::lock_guard<std::mutex> lock(g_conversationMutex);
        auto it = g_conversations.find({ botGuid, otherGuid });
        if (it == g_conversations.end())
            return false;

        ConversationContext& c = it->second;
        const auto now = Clock::now();

        if (c.generation != OllamaConfig_Generation() ||
            c.model != cfg.model || c.url != cfg.url ||
            c.systemPrompt != cfg.systemPrompt || c.numCtx != cfg.numCtx ||
            c.sessionHash != sessionHash)
        {
            g_conversations.erase(it);
            return false;
        }

        if (now - c.lastUsed > kConversationTtl)
        {
            g_conversations.erase(it);
            return false;
        }

        // Two replies for the same pair at once would both start from the same
        // tokens and the second would overwrite the first's turn. The second
        // one uses the full prompt (which contains the last delivered turn),
        // then the conversation continues from there.
        if (c.inFlight)
        {
            if (now - c.inFlightSince > kConversationInFlightTtl)
                c.inFlight = false;
            else
                return false;
        }

        // Leave room for the new turn. A context that no longer fits is
        // dropped rather than truncated: cutting tokens out of the middle of
        // a conversation is worse than rebuilding it from the bounded history.
        if (cfg.numCtx > 0)
        {
            const size_t estimate = deltaChars / 3 + 64;
            if (c.tokens.size() + estimate >= cfg.numCtx)
            {
                g_conversations.erase(it);
                return false;
            }
        }

        c.inFlight      = true;
        c.inFlightSince = now;
        c.lastUsed      = now;
        out             = c.tokens;
        return true;
    }

    void StoreConversation(uint64_t botGuid, uint64_t otherGuid, uint64_t sessionHash,
                           uint32_t generation, std::vector<int> tokens)
    {
        if (!g_ReuseConversationContext || botGuid == 0 || otherGuid == 0)
            return;

        const OllamaEndpointSettings cfg = OllamaConfig_Snapshot();
        if (cfg.provider != OllamaProvider::Ollama)
            return;

        std::lock_guard<std::mutex> lock(g_conversationMutex);
        auto it = g_conversations.find({ botGuid, otherGuid });

        if (tokens.empty())
        {
            if (it != g_conversations.end())
                it->second.inFlight = false;
            return;
        }

        if (cfg.numCtx > 0 && tokens.size() >= cfg.numCtx)
        {
            g_conversations.erase(std::make_pair(botGuid, otherGuid));
            return;
        }

        ConversationContext& c = g_conversations[{ botGuid, otherGuid }];
        c.tokens       = std::move(tokens);
        c.model        = cfg.model;
        c.url          = cfg.url;
        c.systemPrompt = cfg.systemPrompt;
        c.numCtx       = cfg.numCtx;
        c.generation   = generation;
        c.sessionHash  = sessionHash;
        c.inFlight     = false;
        c.lastUsed     = Clock::now();

        if (g_conversations.size() > kMaxConversationContexts)
        {
            auto oldest = g_conversations.begin();
            for (auto it2 = g_conversations.begin(); it2 != g_conversations.end(); ++it2)
                if (it2->second.lastUsed < oldest->second.lastUsed)
                    oldest = it2;
            g_conversations.erase(oldest);
        }
    }

    void ForgetConversation(uint64_t botGuid, uint64_t otherGuid)
    {
        std::lock_guard<std::mutex> lock(g_conversationMutex);
        g_conversations.erase(std::make_pair(botGuid, otherGuid));
    }

    // One generation and its cleanup; "" when nothing usable came back.
    std::string Generate(const Task& task, const std::string& prompt,
                         const std::string& turnPrompt, uint32_t& emoteId, std::string& action)
    {
        action.clear();

        // A stored context turns the request into the next turn of the same
        // conversation: only turnPrompt is sent. Without one (first contact,
        // another provider, an expired or oversized context) the full prompt
        // goes out, exactly as before.
        const uint32_t configGeneration = OllamaConfig_Generation();
        std::vector<int> context;
        bool continued = !turnPrompt.empty() &&
            OllamaConfig_Generation() == configGeneration &&
            TakeConversation(task.request.botGuid, task.request.targetGuid,
                             task.request.sessionHash, turnPrompt.size(), context);

        // A reload between taking the context and the call would pair tokens
        // from the old model with the new one. Rebuild instead.
        if (continued && OllamaConfig_Generation() != configGeneration)
        {
            ForgetConversation(task.request.botGuid, task.request.targetGuid);
            continued = false;
        }

        OllamaApiResult api = continued
            ? QueryOllama(turnPrompt, task.request.kind, 0, &context)
            : QueryOllama(prompt, task.request.kind);

        // A context the server no longer accepts must not cost the bot its
        // reply. Rebuild from the full prompt once, then continue from the
        // context that request returns.
        if (continued && !api.ok && api.status != 429 && api.status != 503)
        {
            if (g_DebugEnabled)
                LOG_INFO("module.ollamachat",
                         "[Ollama Chat] Stored conversation context for {} was rejected ({}); "
                         "falling back to the full prompt.",
                         task.request.botName, api.error.empty() ? "unknown error" : api.error);

            ForgetConversation(task.request.botGuid, task.request.targetGuid);
            api = QueryOllama(prompt, task.request.kind);
        }

        // Keep the newest context even when the reply is unusable: the next
        // turn should continue from the same place the model saw. A successful
        // continued turn that came back without a context cannot be continued,
        // so it is dropped and the next turn rebuilds from the full prompt.
        if (!turnPrompt.empty())
        {
            if (api.ok && api.context.empty())
                ForgetConversation(task.request.botGuid, task.request.targetGuid);
            else
                StoreConversation(task.request.botGuid, task.request.targetGuid,
                                  task.request.sessionHash, configGeneration,
                                  api.ok ? api.context : std::vector<int>{});
        }

        if (!api.ok)
        {
            RecordError(api.error);
            return "";
        }

        // "*grins wickedly* Well met." -- the action becomes an emote line, the rest is spoken.
        std::string raw = api.text;
        action = Coa_TakeActionEmote(raw, task.request.botName);
        if (!action.empty())
        {
            action = Roleplay_FilterMetaTerms(action);
            if (Coa_LooksLikeEcho(action, prompt))
                action.clear();
        }

        std::string text = raw.empty() ? std::string() : ProcessLlmResponse(raw, task.request.botName, &emoteId);

        // Roleplay mode rejects lines carrying out-of-world vocabulary rather
        // than mangling the sentence around the offending word.
        if (!text.empty())
        {
            std::string filtered = Roleplay_FilterMetaTerms(text);
            if (filtered.empty() && !text.empty() && g_DebugEnabled)
            {
                LOG_INFO("module.ollamachat",
                         "[Ollama Chat] Bot {} reply rejected by roleplay filter: '{}'",
                         task.request.botName, text);
            }
            text = std::move(filtered);
        }

        // The model answering with its own instructions (Conquest of Azeroth edition).
        if (!text.empty() && Coa_LooksLikeEcho(text, prompt))
        {
            if (g_DebugEnabled)
                LOG_INFO("module.ollamachat", "[Ollama Chat] Bot {} reply rejected as an echo of its prompt: '{}'",
                         task.request.botName, text);
            text.clear();
        }
        return text;
    }

    void RunChatTask(const Task& task)
    {
        // The memories for this moment are chosen here, on the worker: weighing every one of them by recency,
        // importance and relevance (with embeddings, a call of its own) is no work for the world thread.
        const std::string prompt = Memory_ResolveRecall(task.request.prompt);
        const std::string turnPrompt = task.request.turnPrompt.empty()
            ? std::string()
            : Memory_ResolveRecall(task.request.turnPrompt);

        uint32_t emoteId = 0;
        std::string action;
        std::string text = Generate(task, prompt, turnPrompt, emoteId, action);

        // A line handed over by another module: put its item links back, and fall back to the original where the
        // model lost something of an exact line. Such a line is always said.
        if (task.request.voice)
        {
            bool keep = !text.empty();
            for (size_t i = 0; keep && i < task.request.voiceLinks.size(); ++i)
            {
                const std::string marker = "[" + std::to_string(i + 1) + "]";
                const size_t at = text.find(marker);
                if (at == std::string::npos)
                    keep = false;
                else
                    text.replace(at, marker.size(), task.request.voiceLinks[i]);
            }
            if (keep && task.request.voiceExact)
                keep = Coa_KeepsNumbers(task.request.voiceOriginal, text);
            if (!keep)
            {
                if (g_DebugEnabled)
                    LOG_INFO("module.ollamachat", "[Ollama Chat] Bot {} kept its own words: '{}' (model: '{}')",
                             task.request.botName, task.request.voiceOriginal, text);
                text = task.request.voiceOriginal;
                action.clear();
                emoteId = 0;
            }
        }

        // Someone spoke to this bot directly: an echo is asked again once, and so is an answer that is nearly the
        // same as the last thing the bot said to someone (small models copy their own last line out of the chat
        // history). Ambient chatter is simply dropped instead; the governor handles its repetition.
        if (task.request.directAddress && !task.request.voice)
        {
            std::string last;
            {
                std::lock_guard<std::mutex> lock(g_lastDirectMutex);
                auto it = g_lastDirect.find(task.request.botGuid);
                if (it != g_lastDirect.end())
                    last = it->second;
            }
            bool repeated = !text.empty() && !last.empty() && Governor_Similarity(text, last) >= 0.8f;
            if ((text.empty() && action.empty()) || repeated)
            {
                if (g_DebugEnabled && repeated)
                    LOG_INFO("module.ollamachat", "[Ollama Chat] Bot {} repeated itself ('{}'); asking once more.",
                             task.request.botName, text);
                std::string again = prompt +
                    " Answer with only the words you say, and say something new: do not repeat anything you said before.";
                std::string againTurn = turnPrompt.empty()
                    ? std::string()
                    : turnPrompt + " Answer with only the words you say, and say something new: do not repeat anything you said before.";
                uint32_t emote2 = 0;
                std::string action2;
                std::string second = Generate(task, again, againTurn, emote2, action2);
                if ((!second.empty() || !action2.empty()) && (second.empty() || last.empty() || Governor_Similarity(second, last) < 0.8f))
                {
                    text = std::move(second);
                    action = std::move(action2);
                    emoteId = emote2;
                }
                else if (repeated)
                    text.clear();
            }
            if (!text.empty())
            {
                std::lock_guard<std::mutex> lock(g_lastDirectMutex);
                g_lastDirect[task.request.botGuid] = text;
            }
        }

        if (text.empty() && action.empty())
        {
            ++g_droppedEmpty;
            if (g_DebugEnabled)
                LOG_INFO("module.ollamachat",
                         "[Ollama Chat] Bot {} produced nothing usable after cleanup.",
                         task.request.botName);
            return;
        }

        Completion completion;
        completion.request = task.request;
        completion.text    = std::move(text);
        completion.action  = std::move(action);
        completion.emoteId = emoteId;

        uint32_t delayMs = 0;
        if (g_EnableTypingSimulation)
        {
            delayMs = g_TypingSimulationBaseDelay +
                      static_cast<uint32_t>(completion.text.length()) * g_TypingSimulationDelayPerChar;
            if (g_TypingSimulationMaxDelay > 0 && delayMs > g_TypingSimulationMaxDelay)
                delayMs = g_TypingSimulationMaxDelay;
        }

        // A delay, not a sleep. The old code held a whole thread hostage here.
        completion.deliverAt = Clock::now() + std::chrono::milliseconds(delayMs);

        std::lock_guard<std::mutex> lock(g_doneMutex);
        g_done.push_back(std::move(completion));
    }

    void RunSentimentTask(const Task& task)
    {
        // Sentiment touches only mutex-guarded in-memory state and async DB
        // writes, so it completes here rather than round-tripping to the
        // world thread.
        ApplySentimentAnalysis(task.sentimentBotGuid, task.sentimentPlayerGuid,
                               task.sentimentMessage, task.sentimentPrompt);
    }

    void WorkerLoop()
    {
        for (;;)
        {
            Task task;

            {
                std::unique_lock<std::mutex> lock(g_queueMutex);
                g_queueCv.wait(lock, [] { return !g_running || !g_queue.empty(); });

                if (!g_running && g_queue.empty())
                    return;

                task = std::move(g_queue.front());
                g_queue.pop_front();
            }

            ++g_inFlight;

            try
            {
                switch (task.type)
                {
                    case TaskType::Sentiment:
                        RunSentimentTask(task);
                        break;
                    case TaskType::Condense:
                        Memory_RunCondensation(task.memoryBotGuid, task.memoryPrompt);
                        break;
                    case TaskType::Relationship:
                        Memory_RunRelationshipUpdate(task.memoryBotGuid, task.memoryOtherGuid,
                                                     task.memoryOtherName, task.memoryPrompt);
                        break;
                    case TaskType::Job:
                        if (task.job)
                            task.job();
                        break;
                    default:
                        RunChatTask(task);
                        break;
                }
            }
            catch (const std::exception& e)
            {
                RecordError(e.what());
                LOG_ERROR("module.ollamachat", "[Ollama Chat] Worker exception: {}", e.what());
            }
            catch (...)
            {
                RecordError("unknown exception");
                LOG_ERROR("module.ollamachat", "[Ollama Chat] Unknown worker exception.");
            }

            --g_inFlight;
        }
    }

    // --- delivery (world thread only) -------------------------------------

    Channel* ResolveChannel(Player* bot, const std::string& channelName)
    {
        if (!bot || channelName.empty())
            return nullptr;

        ChannelMgr* mgr = ChannelMgr::forTeam(bot->GetTeamId());
        if (!mgr)
            return nullptr;

        return mgr->GetChannel(channelName, bot);
    }

    bool AnyoneInRange(Player* bot, float distance)
    {
        if (!bot || !bot->IsInWorld() || distance <= 0.0f)
            return false;

        // Grid search: this runs on every Say/Yell delivery, and walking every
        // online character to answer "is anyone standing near me" is the kind
        // of thing that adds up on a bot-heavy realm.
        std::list<Player*> found;
        Acore::AnyPlayerInObjectRangeCheck check(bot, distance, false, true);
        Acore::PlayerListSearcher<Acore::AnyPlayerInObjectRangeCheck> searcher(bot, found, check);
        Cell::VisitObjects(bot, searcher, distance);

        for (Player* other : found)
            if (other && other != bot && other->IsInWorld())
                return true;

        return false;
    }

    // Guild lines go out here, not by PlayerbotAI::SayToGuild: that one is shut by AiPlayerbot.CoaLive.Chat.Enable = 0,
    // which AFK Realm sets so that only the language model talks.
    bool SayToGuild(Player* bot, const std::string& text)
    {
        if (text.empty() || !bot->GetGuildId())
            return false;
        Guild* guild = sGuildMgr->GetGuildById(bot->GetGuildId());
        if (!guild || !guild->HasRankRight(bot, GR_RIGHT_GCHATSPEAK))
            return false;
        guild->BroadcastToGuild(bot->GetSession(), false, text, LANG_UNIVERSAL);
        return true;
    }

    // Returns true when the line actually went out.
    bool RouteMessage(Player* bot, PlayerbotAI* botAI, const Completion& c,
                      const OllamaWorldSnapshot& world,
                      Channel*& outChannel)
    {
        outChannel = nullptr;

        switch (c.request.source)
        {
            case SRC_GENERAL_LOCAL:
            {
                Channel* channel = ResolveChannel(bot, c.request.channelName);
                if (!channel || !OllamaIsOnChannel(channel, bot->GetGUID()))
                    return false;

                // Checked before generating too; re-checked because the only
                // human in the channel can leave during the LLM round trip.
                if (!world.RealPlayerInChannel(channel))
                    return false;

                channel->Say(bot->GetGUID(), c.text, LANG_UNIVERSAL);
                outChannel = channel;
                return true;
            }

            // Guild, party and raid are re-checked here for the same reason
            // say and yell always were: the audience is validated at submit
            // time, and an LLM round trip is seconds long. Whoever the bot was
            // talking to can log out, leave the guild or drop group in that
            // window, and without this the bot announces to an empty channel.
            case SRC_GUILD_LOCAL:
            case SRC_OFFICER_LOCAL:
                if (g_DisableForGuild || !bot->GetGuild())
                    return false;
                if (!world.GuildHasRealPlayer(bot->GetGuildId()))
                    return false;
                return SayToGuild(bot, c.text);

            case SRC_PARTY_LOCAL:
                if (g_DisableForParty || !bot->GetGroup())
                    return false;
                if (!OllamaGroupHasRealPlayer(bot))
                    return false;
                Voice_Label(bot, nullptr, VOICE_PARTY, c.text);
                return botAI->SayToParty(c.text);

            case SRC_RAID_LOCAL:
                if (g_DisableForParty || !bot->GetGroup())
                    return false;
                if (!OllamaGroupHasRealPlayer(bot))
                    return false;
                Voice_Label(bot, nullptr, VOICE_PARTY, c.text);
                return botAI->SayToRaid(c.text);

            case SRC_YELL_LOCAL:
                if (g_DisableForSayYell || !AnyoneInRange(bot, g_YellDistance))
                    return false;
                Voice_Label(bot, nullptr, VOICE_YELL, c.text);
                return botAI->Yell(c.text);

            case SRC_WHISPER_LOCAL:
            {
                Player* target = ObjectAccessor::FindConnectedPlayer(ObjectGuid(c.request.targetGuid));
                if (!target)
                    return false;
                Voice_Label(bot, target, VOICE_WHISPER, c.text);
                return botAI->Whisper(c.text, target->GetName());
            }

            case SRC_SAY_LOCAL:
            default:
                if (g_DisableForSayYell || !AnyoneInRange(bot, g_SayDistance))
                    return false;
                Voice_Label(bot, nullptr, VOICE_SAY, c.text);
                return botAI->Say(c.text);
        }
    }

    // A reply as the chat lines that carry it whole: the client shows at most 255 bytes a line, so a longer reply
    // goes out in several, split after a sentence (else at a word). Cut off by num_predict, only the unfinished tail goes.
    std::vector<std::string> ChatLines(std::string s)
    {
        const size_t kMax = 255;
        auto isEnd = [](char ch) { return ch == '.' || ch == '!' || ch == '?' || ch == '*' || ch == '"' || ch == '~'; };
        auto sentenceEnd = [&](const std::string& t, size_t limit, size_t keepAtLeast) -> size_t
        {
            for (size_t i = std::min(limit, t.size()); i > keepAtLeast; --i)
                if (isEnd(t[i - 1]) && (i == t.size() || t[i] == ' '))
                    return i;
            return 0;
        };
        while (!s.empty() && std::isspace(static_cast<unsigned char>(s.back())))
            s.pop_back();
        if (!s.empty() && !isEnd(s.back()) && s.back() != '\'' && s.back() != ')')
        {
            if (size_t at = sentenceEnd(s, s.size(), 0))
                s.resize(at);
            else
                s += s.size() > 60 ? "..." : ".";   // a long line without an end was most likely cut off
        }
        std::vector<std::string> lines;
        while (s.size() > kMax)
        {
            size_t at = sentenceEnd(s, kMax, kMax / 3);
            if (!at)
                at = s.rfind(' ', kMax);
            if (at == std::string::npos || at < kMax / 3)
            {
                at = kMax;
                while (at > 0 && (static_cast<unsigned char>(s[at]) & 0xC0) == 0x80)   // not inside a UTF-8 letter
                    --at;
            }
            lines.push_back(s.substr(0, at));
            size_t next = s.find_first_not_of(' ', at);
            s = next == std::string::npos ? std::string() : s.substr(next);
        }
        if (!s.empty())
            lines.push_back(s);
        return lines;
    }

    // Each line of a reply through the same route, in order.
    bool RouteLines(Player* bot, PlayerbotAI* botAI, const Completion& c, const OllamaWorldSnapshot& world,
                    Channel*& outChannel)
    {
        bool sent = false;
        Completion line = c;
        for (const std::string& part : ChatLines(c.text))
        {
            line.text = part;
            if (!RouteMessage(bot, botAI, line, world, outChannel))
                break;
            sent = true;
        }
        return sent;
    }

    void Deliver(const Completion& delivered, const OllamaWorldSnapshot& world)
    {
        Completion c = delivered;

        // Where the reply carried an action, it goes out as an emote line ("Klaus grins wickedly.") in say and yell,
        // where the people around see it. Party, guild, channels and whispers have no emote line, so there the
        // action stays in front of the words between asterisks.
        const bool local = c.request.source == SRC_SAY_LOCAL || c.request.source == SRC_YELL_LOCAL;
        if (!c.action.empty() && !local)
        {
            c.text = "*" + c.action + "*" + (c.text.empty() ? "" : " " + c.text);
            c.action.clear();
        }

        Player* bot = ObjectAccessor::FindConnectedPlayer(ObjectGuid(c.request.botGuid));
        if (!bot || !bot->IsInWorld())
            return;

        PlayerbotAI* botAI = PlayerbotsMgr::instance().GetPlayerbotAI(bot);
        if (!botAI)
            return;

        const ObjectGuid botGuid = bot->GetGUID();

        // Direct address: the bot owes this person an answer, so it skips the
        // suppressions that exist to pace ambient chatter. Decided at submit
        // time in ProcessChat, where the Group was live to consult.
        const bool directAddress = c.request.directAddress;

        // Repetition is checked at delivery rather than at submission, because
        // we only know what the model actually said now.
        //
        // Skipped for direct address. Ask a bot the same question twice and
        // the same answer is correct -- suppressing it leaves the asker
        // staring at silence, which reads as the bot being broken rather than
        // as anti-repetition working.
        if (!c.request.voice && !directAddress && !c.text.empty() &&
            Governor_IsRepetitive(botGuid, c.request.scopeKey, c.text))
        {
            ++g_droppedGovernor;
            if (g_DebugEnabled)
                LOG_INFO("module.ollamachat",
                         "[Ollama Chat] Bot {} reply suppressed as repetitive: '{}'",
                         bot->GetName(), c.text);
            return;
        }

        if (!c.request.voice && !Governor_TryConsumeSend(botGuid, c.request.scopeKey, directAddress))
        {
            ++g_droppedGovernor;
            if (g_DebugEnabled)
                LOG_INFO("module.ollamachat",
                         "[Ollama Chat] Bot {} reply suppressed by cooldown/rate limit.",
                         bot->GetName());
            return;
        }

        Channel* channel = nullptr;
        bool sent = false;
        if (!c.action.empty() && !g_DisableForSayYell && AnyoneInRange(bot, g_SayDistance))
        {
            bot->TextEmote(c.action);
            sent = true;
        }
        if (!c.text.empty() && c.request.voice && c.request.source == SRC_WHISPER_LOCAL)
        {
            // As the module would have whispered it: in the universal tongue, so a trade across the factions reads.
            if (Player* to = ObjectAccessor::FindConnectedPlayer(ObjectGuid(c.request.targetGuid)))
            {
                for (const std::string& part : ChatLines(c.text))
                {
                    Voice_Label(bot, to, VOICE_WHISPER, part);
                    bot->Whisper(part, LANG_UNIVERSAL, to);
                }
                sent = true;
            }
        }
        else if (!c.text.empty())
            sent = RouteLines(bot, botAI, c, world, channel) || sent;
        if (!sent)
        {
            if (g_DebugEnabled)
                LOG_INFO("module.ollamachat",
                         "[Ollama Chat] Bot {} had nowhere to send its reply ({}).",
                         bot->GetName(), ChatChannelSourceLocalStr[c.request.source]);
            return;
        }

        // What the bot did and said, for its memory of the conversation.
        const std::string remembered = c.action.empty() ? c.text
                                     : "*" + c.action + "*" + (c.text.empty() ? "" : " " + c.text);

        if (!c.text.empty())
            Governor_RecordUtterance(botGuid, c.request.scopeKey, c.text);
        ++g_totalDelivered;

        // This bot is now in a conversation with whoever it just answered, so
        // their next line in this scope is a turn in it rather than ambient
        // chatter. Only against a real person: an open conversation bypasses
        // pacing, and letting two bots open one with each other is how a
        // bot-to-bot loop would escape every brake in the governor.
        if (c.request.targetGuid)
        {
            Player* addressee = ObjectAccessor::FindConnectedPlayer(ObjectGuid(c.request.targetGuid));
            // Two bots talking: each may pass on something it went through with someone (rumours spread).
            if (addressee && addressee != bot && OllamaIsBotPlayer(addressee))
            {
                Memory_ShareRumour(addressee, bot);
                Memory_ShareRumour(bot, addressee);
            }
            if (OllamaIsRealPlayer(addressee) && !c.request.voice)
            {
                Governor_NoteConversation(botGuid, ObjectGuid(c.request.targetGuid),
                                          c.request.scopeKey);
                Presence_AfterReply(bot, addressee);
            }
        }

        // Body language. Safe here and only here: this is the world thread.
        ScheduleBotExpression(bot, ObjectGuid(c.request.targetGuid), c.emoteId,
                              g_BotExpressionDelayMs);

        if (c.request.recordHistory && c.request.targetGuid)
        {
            AppendBotConversation(c.request.botGuid, c.request.targetGuid,
                                  c.request.originMessage, remembered);

            // Counts name mentions and, past a threshold, queues a
            // condensation or relationship revision. World thread.
            Player* target = ObjectAccessor::FindConnectedPlayer(ObjectGuid(c.request.targetGuid));
            if (target && OllamaIsRealPlayer(target))
                Bond_NoteTalk(bot, target);
            Memory_NoteExchange(c.request.botGuid, c.request.targetGuid,
                                target ? target->GetName() : std::string(),
                                c.request.originMessage, remembered);
        }

        if (c.request.updateSentiment && c.request.targetGuid &&
            !c.request.originMessage.empty())
        {
            OllamaDispatch_SubmitSentiment(c.request.botGuid, c.request.targetGuid,
                                           c.request.originMessage);
        }

        if (g_DebugEnabled)
            LOG_INFO("module.ollamachat", "[Ollama Chat] {} ({}, depth {}): {}",
                     bot->GetName(), ChatChannelSourceLocalStr[c.request.source],
                     c.request.chainDepth, remembered);

        // Let other bots hear it -- with the chain depth advanced, which is
        // what stops the reply loop that had no brakes before.
        if (c.request.triggerBotReplies && !c.text.empty() &&
            c.request.source != SRC_WHISPER_LOCAL)
        {
            ProcessBotChatMessage(bot, c.text, c.request.source, channel,
                                  static_cast<uint8_t>(c.request.chainDepth + 1));
        }
    }
}

// --------------------------------------------------------------------------

void OllamaDispatch_Start()
{
    std::lock_guard<std::mutex> lock(g_queueMutex);
    if (g_running)
        return;

    g_running = true;

    uint32_t count = g_DispatchWorkerThreads;
    if (count == 0)
        count = 4;
    if (count > 64)
        count = 64;

    g_workers.reserve(count);
    for (uint32_t i = 0; i < count; ++i)
        g_workers.emplace_back(WorkerLoop);

    LOG_INFO("module.ollamachat", "[Ollama Chat] Dispatcher started with {} worker threads.", count);
}

void OllamaDispatch_Stop()
{
    {
        std::lock_guard<std::mutex> lock(g_queueMutex);
        if (!g_running)
            return;
        g_running = false;
        g_queue.clear();
    }

    g_queueCv.notify_all();

    for (auto& worker : g_workers)
        if (worker.joinable())
            worker.join();

    g_workers.clear();

    {
        std::lock_guard<std::mutex> lock(g_doneMutex);
        g_done.clear();
    }

    LOG_INFO("module.ollamachat", "[Ollama Chat] Dispatcher stopped.");
}

bool OllamaDispatch_Submit(OllamaChatRequest request)
{
    if (request.prompt.empty() || request.botGuid == 0)
        return false;

    // OllamaChat.OnlyPartyBots: only bots grouped with a real player use the language model
    if (g_OnlyPartyBots)
    {
        Player* bot = ObjectAccessor::FindPlayer(ObjectGuid(request.botGuid));
        if (!bot || !bot->GetGroup() || !OllamaGroupHasRealPlayer(bot))
            return false;
    }

    Task task;
    task.type    = TaskType::ChatReply;
    task.request = std::move(request);

    {
        std::lock_guard<std::mutex> lock(g_queueMutex);

        if (!g_running)
            return false;

        if (g_MaxQueueDepth > 0 && g_queue.size() >= g_MaxQueueDepth)
        {
            ++g_droppedQueueFull;
            if (g_DebugEnabled)
                LOG_INFO("module.ollamachat",
                         "[Ollama Chat] Queue full ({}); dropping request for {}.",
                         g_queue.size(), task.request.botName);
            return false;
        }

        g_queue.push_back(std::move(task));
    }

    ++g_totalSubmitted;
    g_queueCv.notify_one();
    return true;
}

void OllamaDispatch_ForgetConversation(uint64_t botGuid, uint64_t targetGuid)
{
    ForgetConversation(botGuid, targetGuid);
}

void OllamaDispatch_ForgetConversations(uint64_t guid)
{
    if (guid == 0)
        return;

    std::lock_guard<std::mutex> lock(g_conversationMutex);
    for (auto it = g_conversations.begin(); it != g_conversations.end(); )
    {
        if (it->first.first == guid || it->first.second == guid)
            it = g_conversations.erase(it);
        else
            ++it;
    }
}

void OllamaDispatch_SubmitSentiment(uint64_t botGuid, uint64_t playerGuid,
                                    const std::string& message)
{
    if (!g_EnableSentimentTracking || message.empty())
        return;

    // Built here, on the world thread: the template is config state that
    // reload rewrites, so a worker must never read it.
    std::string prompt = BuildSentimentPrompt(message);
    if (prompt.empty())
        return;

    Task task;
    task.type                = TaskType::Sentiment;
    task.sentimentBotGuid    = botGuid;
    task.sentimentPlayerGuid = playerGuid;
    task.sentimentMessage    = message;
    task.sentimentPrompt     = std::move(prompt);

    {
        std::lock_guard<std::mutex> lock(g_queueMutex);
        if (!g_running)
            return;

        // Sentiment is best-effort; never let it crowd out actual chat.
        if (g_MaxQueueDepth > 0 && g_queue.size() >= g_MaxQueueDepth / 2)
            return;

        g_queue.push_back(std::move(task));
    }

    g_queueCv.notify_one();
}

void OllamaDispatch_Update(uint32_t /*diff*/)
{
    const auto now = Clock::now();

    // Move due completions out under the lock, then deliver without it: the
    // delivery path re-enters ProcessBotChatMessage, which submits new work.
    std::vector<Completion> due;

    {
        std::lock_guard<std::mutex> lock(g_doneMutex);

        for (auto it = g_done.begin(); it != g_done.end(); )
        {
            if (it->deliverAt <= now)
            {
                due.push_back(std::move(*it));
                it = g_done.erase(it);
            }
            else
            {
                ++it;
            }
        }
    }

    if (due.empty())
        return;

    // One pass over the online player list for the whole tick, rather than one
    // per delivery.
    OllamaWorldSnapshot world;
    world.Build();

    for (const Completion& c : due)
    {
        try
        {
            Deliver(c, world);
        }
        catch (const std::exception& e)
        {
            LOG_ERROR("module.ollamachat", "[Ollama Chat] Delivery exception: {}", e.what());
        }
    }
}

void OllamaChat_DispatchEmoteReaction(Player* bot, Player* player, uint32_t textEmote)
{
    if (!bot || !player)
        return;

    OllamaChatRequest request;
    request.botGuid    = bot->GetGUID().GetRawValue();
    request.targetGuid = player->GetGUID().GetRawValue();
    request.source     = SRC_SAY_LOCAL;
    request.chainDepth = 0;
    request.botName    = bot->GetName();
    request.kind       = OllamaRequestKind::EventChatter;
    request.scopeKey   = Governor_MakeScopeKey("Say", 0, "", 0, bot->GetZoneId());
    request.triggerBotReplies = false;
    // Someone emoted at this bot by name. Its own debounce paces this;
    // the ambient say cooldown has no business also silencing it.
    request.directAddress = true;

    request.prompt = BuildEmoteReactionPrompt(bot, player, textEmote);
    if (request.prompt.empty())
        return;

    OllamaDispatch_Submit(std::move(request));
}

namespace
{
    // Background upkeep must never crowd out actual chat, so it gets a
    // stricter queue allowance than a reply does.
    bool SubmitBackground(Task&& task)
    {
        std::lock_guard<std::mutex> lock(g_queueMutex);
        if (!g_running)
            return false;
        if (g_MaxQueueDepth > 0 && g_queue.size() >= g_MaxQueueDepth / 2)
            return false;

        g_queue.push_back(std::move(task));
        return true;
    }
}

bool OllamaDispatch_SubmitJob(std::function<void()> job)
{
    if (!job)
        return false;
    Task task;
    task.type = TaskType::Job;
    task.job  = std::move(job);
    if (!SubmitBackground(std::move(task)))
        return false;
    g_queueCv.notify_one();
    return true;
}

void OllamaDispatch_SubmitCondensation(uint64_t botGuid, const std::string& prompt)
{
    if (botGuid == 0 || prompt.empty())
        return;

    Task task;
    task.type          = TaskType::Condense;
    task.memoryBotGuid = botGuid;
    task.memoryPrompt  = prompt;

    if (SubmitBackground(std::move(task)))
        g_queueCv.notify_one();
}

void OllamaDispatch_SubmitRelationship(uint64_t botGuid, uint64_t otherGuid,
                                       const std::string& otherName,
                                       const std::string& prompt)
{
    if (botGuid == 0 || otherGuid == 0 || prompt.empty())
        return;

    Task task;
    task.type            = TaskType::Relationship;
    task.memoryBotGuid   = botGuid;
    task.memoryOtherGuid = otherGuid;
    task.memoryOtherName = otherName;
    task.memoryPrompt    = prompt;

    if (SubmitBackground(std::move(task)))
        g_queueCv.notify_one();
}

OllamaDispatchStats OllamaDispatch_GetStats()
{
    OllamaDispatchStats stats{};

    {
        std::lock_guard<std::mutex> lock(g_queueMutex);
        stats.queuedRequests = static_cast<uint32_t>(g_queue.size());
        stats.workers        = static_cast<uint32_t>(g_workers.size());
    }
    {
        std::lock_guard<std::mutex> lock(g_doneMutex);
        stats.pendingDeliveries = static_cast<uint32_t>(g_done.size());
    }
    {
        std::lock_guard<std::mutex> lock(g_errorMutex);
        stats.lastError = g_lastError;
    }

    stats.inFlight              = g_inFlight.load();
    stats.totalSubmitted        = g_totalSubmitted.load();
    stats.totalDelivered        = g_totalDelivered.load();
    stats.totalDroppedQueueFull = g_droppedQueueFull.load();
    stats.totalDroppedEmpty     = g_droppedEmpty.load();
    stats.totalDroppedGovernor  = g_droppedGovernor.load();
    stats.totalFailed           = g_totalFailed.load();

    return stats;
}

// Explicit template instantiations are exempt from access checking
// ([temp.spec.general]), so instantiating this with &Channel::IsOn is legal
// and hands the member pointer out through a friend function. If the core
// ever changes IsOn's signature, this stops compiling rather than misbehaving.
namespace
{
    using ChannelIsOnFn = bool (Channel::*)(ObjectGuid) const;

    ChannelIsOnFn ChannelIsOnPtr();

    template <ChannelIsOnFn Fn>
    struct ChannelIsOnAccess
    {
        friend ChannelIsOnFn ChannelIsOnPtr() { return Fn; }
    };

    template struct ChannelIsOnAccess<&Channel::IsOn>;
}

bool OllamaIsOnChannel(Channel const* channel, ObjectGuid guid)
{
    return channel && (channel->*ChannelIsOnPtr())(guid);
}

Channel* OllamaResolveZoneChannel(Player* bot, uint32_t chatChannelId)
{
    if (!bot)
        return nullptr;

    ChannelMgr* mgr = ChannelMgr::forTeam(bot->GetTeamId());
    if (!mgr)
        return nullptr;

    PlayerbotAI* botAI = PlayerbotsMgr::instance().GetPlayerbotAI(bot);
    std::string zoneName;
    if (botAI)
        if (AreaTableEntry const* zone = botAI->GetCurrentZone())
            zoneName = PlayerbotAI::GetLocalizedAreaName(zone);

    for (auto const& [key, channel] : mgr->GetChannels())
    {
        if (!channel || channel->GetName().empty())
            continue;
        if (channel->GetChannelId() != chatChannelId)
            continue;

        // Global channels (LFG, WorldDefense) are not zone-scoped.
        const bool zoneScoped = (chatChannelId != uint32_t(ChatChannelId::LOOKING_FOR_GROUP) &&
                                 chatChannelId != uint32_t(ChatChannelId::WORLD_DEFENSE));

        if (zoneScoped)
        {
            if (zoneName.empty())
                continue;
            if (channel->GetName().find(zoneName) == std::string::npos)
                continue;
        }

        return channel;
    }

    return nullptr;
}
