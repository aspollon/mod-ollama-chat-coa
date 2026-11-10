#ifndef MOD_OLLAMA_CHAT_MEMORY_H
#define MOD_OLLAMA_CHAT_MEMORY_H

#include "ObjectGuid.h"
#include <string>
#include <vector>
#include <cstdint>

class Player;

// --------------------------------------------------------------------------
// Long-term memory and relationships.
//
// Conversation history alone is a sliding window: once a line falls out of it,
// the bot has no idea it ever happened. That is why bots feel like they meet
// you fresh every session no matter how much you have talked.
//
// Two mechanisms fix that, both bounded so the prompt never grows without
// limit:
//
//   MEMORY        History accumulates until it crosses a token budget. At that
//                 point it is condensed by the model into a handful of short
//                 narrator-style memories, each with an importance score, and
//                 the raw history is cleared. At prompt-build time the most
//                 important memories are selected within a separate, smaller
//                 token budget.
//
//   RELATIONSHIPS When a name comes up often enough in a bot's history, the
//                 model is asked to write (or revise) a sentence on how that
//                 bot feels about that person. That sentence goes into future
//                 prompts, so a bot's attitude persists and evolves instead of
//                 resetting.
//
// Both work in normal mode and roleplay mode; roleplay mode simply asks for
// in-character phrasing and allows a larger budget.
//
// Sentiment tracking (a single number) is complementary and unchanged: this is
// the qualitative half.
// --------------------------------------------------------------------------

struct BotMemoryEntry
{
    std::string text;
    uint8_t     importance = 5;   // 1..10
    uint64_t    createdAt  = 0;   // unix seconds
    uint64_t    aboutGuid  = 0;   // an experience with this person; the bot's own guid for its own doings; 0 for notes
    uint8_t     kind       = 0;   // MemoryKind
    uint64_t    lastAccess = 0;   // last time it came up in a prompt (recency decays from here); not stored
};

// After "Generative Agents" (Park et al., 2023).
enum MemoryKind : uint8_t
{
    MEMORY_EXPERIENCE = 0,        // something that happened, or a note from a conversation
    MEMORY_REFLECTION = 1,        // what the bot concluded from it
    MEMORY_GOAL       = 2,        // what the bot wants in life
    MEMORY_RUMOUR     = 3         // what another bot told it about someone
};

struct BotRelationship
{
    uint64_t    otherGuid = 0;
    std::string otherName;
    std::string description;
    uint32_t    mentions  = 0;
    uint64_t    updatedAt = 0;
};

void Memory_Load();
void Memory_SaveAll();
void Memory_ForgetBot(ObjectGuid botGuid);

// Called on the world thread after a bot exchange is recorded. Counts name
// mentions and, when a threshold is crossed, queues a condensation or a
// relationship update through the dispatcher.
void Memory_NoteExchange(uint64_t botGuid, uint64_t otherGuid,
                         const std::string& otherName,
                         const std::string& incomingMessage,
                         const std::string& botReply);

// Prompt fragments. World thread only.
//
// `about` may be null; when set, that person's relationship line is listed
// first so the bot's attitude to whoever it is talking to always survives the
// budget.
std::string Memory_BuildPromptSection(Player* bot, Player* about);

// Conquest of Azeroth: something the bot went through ("You and Chris killed Hogger in Elwynn Forest."), with a
// person (`aboutGuid`) or alone (its own guid). Kept with its other memories, so the same limits apply. World thread.
void Memory_AddExperience(uint64_t botGuid, uint64_t aboutGuid, const std::string& text, uint8_t importance,
                          uint8_t kind = MEMORY_EXPERIENCE);

// The memory section of a prompt is chosen on the worker, where there is time to weigh every memory by how recent,
// how important and how relevant to the moment it is (with an embedding model, by meaning). Prompts carry a marker
// for it; Memory_ResolveRecall puts the section in its place.
std::string Memory_RecallMarker(Player* bot, Player* about, const std::string& query);
std::string Memory_ResolveRecall(const std::string& prompt);   // worker thread

// World thread, every few seconds: reflections and goals for bots that have dealt with real people.
void Memory_Update();

// Two bots talked: the listener may hear something the teller went through with someone (world thread).
void Memory_ShareRumour(Player* teller, Player* listener);

// --- worker-side entry points (called from the dispatcher) ----------------

// Condense a bot's accumulated history into memories, then clear the history.
void Memory_RunCondensation(uint64_t botGuid, const std::string& prompt);

// Write or revise how a bot feels about someone.
void Memory_RunRelationshipUpdate(uint64_t botGuid, uint64_t otherGuid,
                                  const std::string& otherName,
                                  const std::string& prompt);

// --- prompt builders (world thread; they read config templates) -----------

std::string Memory_BuildCondensationPrompt(Player* bot);
std::string Memory_BuildRelationshipPrompt(Player* bot, uint64_t otherGuid,
                                           const std::string& otherName);

// Rough token estimate. Deliberately cheap: characters / 4, the usual
// approximation, which is accurate enough for a budget.
uint32_t Memory_EstimateTokens(const std::string& text);

#endif // MOD_OLLAMA_CHAT_MEMORY_H
