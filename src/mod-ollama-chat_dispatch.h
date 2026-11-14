#ifndef MOD_OLLAMA_CHAT_DISPATCH_H
#define MOD_OLLAMA_CHAT_DISPATCH_H

#include "mod-ollama-chat_capability.h"
#include "mod-ollama-chat_handler.h"

#include "ObjectGuid.h"
#include <string>
#include <cstdint>
#include <functional>
#include <vector>

class Player;
class Channel;

// --------------------------------------------------------------------------
// Central dispatcher.
//
// THE RULE THIS FILE EXISTS TO ENFORCE:
//   worker threads do HTTP and string work only.
//   every read or write of a Player, Channel, Guild, Group or Map happens on
//   the world thread.
//
// Previously each candidate bot spawned its own detached std::thread which
// then called ObjectAccessor, Channel::Say, botAI->Say and re-entered the
// whole eligibility scan -- all racing Map::Update, and all unbounded.
//
// Now: the caller builds the prompt on the world thread and submits. A fixed
// pool of workers performs the generation. Completions come back through a
// queue that OllamaDispatch_Update() drains on the world thread, which is the
// only place delivery happens.
// --------------------------------------------------------------------------

// Resolve the zone-scoped instance of a numbered channel (General, Trade, ...)
// for this bot.
//
// ChannelMgr keys channels by their FULL name, and zone channels are named
// "General - Elwynn Forest", not "General" -- so GetChannel("General") always
// returns nullptr. Matching is by channel id plus zone-name containment, the
// same way PlayerbotAI::SayToChannel does it.
//
// World thread only.
Channel* OllamaResolveZoneChannel(Player* bot, uint32_t chatChannelId);

// Is this guid a member of this exact channel instance?
//
// The playerbots fork dropped Player::IsInChannel, and the core keeps
// Channel::IsOn private with no public replacement. This reaches IsOn itself
// so membership stays exact, including custom (unnumbered) channels.
//
// World thread only.
bool OllamaIsOnChannel(Channel const* channel, ObjectGuid guid);

struct OllamaChatRequest
{
    // Who speaks, and to whom.
    uint64_t botGuid    = 0;
    uint64_t targetGuid = 0;      // 0 when there is no specific addressee

    // Where the line goes.
    ChatChannelSourceLocal source = SRC_SAY_LOCAL;
    std::string            channelName;
    uint32_t               channelId = 0;

    // Loop control. Human-originated messages start at depth 0; each bot reply
    // to a bot increments it.
    uint8_t     chainDepth = 0;
    std::string scopeKey;

    // This line was aimed at this bot in particular -- whispered, named, or
    // said in a small party it belongs to -- rather than said to the room.
    // Decided on the world thread at submit time, where the Group is live, and
    // carried here so delivery does not have to guess it back from `source`.
    // Direct address skips the pacing cooldowns and repetition suppression.
    bool directAddress = false;

    // Generation.
    std::string       prompt;
    // The per-turn part of the prompt. When a stored Ollama context exists,
    // only this part is sent; the character sheet, lore, voice and the talk so
    // far are already in the context. Empty means "always send the full prompt".
    std::string       turnPrompt;
    // Hash of the static part the context was built from. A level-up, zone
    // change or edited roleplay sheet makes the context stale.
    uint64_t          sessionHash = 0;
    OllamaRequestKind kind = OllamaRequestKind::ChatReply;

    // Resolved on the world thread before submission so the worker never has
    // to touch a Player to know these.
    std::string botName;
    std::string originMessage;    // the message being replied to, if any

    // Post-delivery behaviour.
    bool triggerBotReplies = true;   // let other bots hear this line
    bool recordHistory     = false;  // append to conversation history

    // A line another module handed over to be said in the bot's own words (CoaChatBridge::Speak). It is always
    // said: if the model fails, or drops an item, a number or a marker of an exact line, the original goes out.
    bool                     voice = false;
    bool                     voiceExact = true;
    std::string              voiceOriginal;   // the line as the module wrote it, item links and all
    std::vector<std::string> voiceLinks;      // the links taken out of it, standing as [1], [2] ... in the prompt
};

// Submit a request. Returns false when the queue is at MaxQueueDepth, in which
// case the caller should simply skip this bot rather than pile up backlog.
bool OllamaDispatch_Submit(OllamaChatRequest request);

// Drop the stored conversation context for one pair or for everything a
// character is part of. A context is a snapshot of one conversation; when the
// history is deleted or a character leaves, it must not be continued.
void OllamaDispatch_ForgetConversation(uint64_t botGuid, uint64_t targetGuid);
void OllamaDispatch_ForgetConversations(uint64_t guid);

// Drain finished generations and deliver them. World thread only.
void OllamaDispatch_Update(uint32_t diff);

void OllamaDispatch_Start();
void OllamaDispatch_Stop();

// Background work for a worker thread (a summary, say); it may call QueryOllama. Dropped when the queue is busy.
bool OllamaDispatch_SubmitJob(std::function<void()> job);

void OllamaChat_DispatchEmoteReaction(Player* bot, Player* player, uint32_t textEmote);

struct OllamaDispatchStats
{
    uint32_t queuedRequests;
    uint32_t inFlight;
    uint32_t pendingDeliveries;
    uint32_t workers;
    uint64_t totalSubmitted;
    uint64_t totalDelivered;
    uint64_t totalDroppedQueueFull;
    uint64_t totalDroppedEmpty;
    uint64_t totalDroppedGovernor;
    uint64_t totalFailed;
    std::string lastError;
};
OllamaDispatchStats OllamaDispatch_GetStats();

#endif // MOD_OLLAMA_CHAT_DISPATCH_H
