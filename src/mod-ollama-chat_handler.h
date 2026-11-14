#ifndef MOD_OLLAMA_CHAT_HANDLER_H
#define MOD_OLLAMA_CHAT_HANDLER_H

#include "ScriptMgr.h"
#include <string>
#include <cstdint>

class Channel;
class Player;

enum ChatChannelSourceLocal
{
    SRC_UNDEFINED_LOCAL  = 0,
    SRC_SAY_LOCAL        = 1,
    SRC_PARTY_LOCAL      = 2,
    SRC_RAID_LOCAL       = 3,
    SRC_GUILD_LOCAL      = 4,
    SRC_OFFICER_LOCAL    = 5,
    SRC_YELL_LOCAL       = 6,
    SRC_WHISPER_LOCAL    = 7,
    SRC_GENERAL_LOCAL    = 17
};

extern const char* ChatChannelSourceLocalStr[];

std::string rtrim(const std::string& s);
ChatChannelSourceLocal GetChannelSourceLocal(uint32_t type);

// Called after a bot has spoken so other bots can hear it.
//
// chainDepth is how many bot->bot hops led here; a message from a real player
// is depth 0. The governor refuses beyond MaxChainDepth and decays the reply
// chance at each hop, which is what stops the endless bot conversations.
void ProcessBotChatMessage(Player* bot, const std::string& msg,
                           ChatChannelSourceLocal sourceLocal, Channel* channel,
                           uint8_t chainDepth = 1);

void SaveBotConversationHistoryToDB();
void DeleteBotConversationHistoryFromDB(uint64_t botGuid);
void AppendBotConversation(uint64_t botGuid, uint64_t playerGuid,
                           const std::string& playerMessage, const std::string& botReply);

// Prompt builders. World thread only -- they read live world state.
//
// The chat prompt is built in two parts. `staticPrompt` is everything that
// stays the same for the whole conversation (identity, personality, lore,
// voice, roleplay sheets, gesture rules); `turn` is what changed or is new
// (situation, message-specific lore, the new line). A first contact sends
// both. A continued conversation sends only `turn` and lets Ollama continue
// from its stored token context.
struct BotPromptParts
{
    std::string staticPrompt;
    std::string history;        // past turns, may be empty
    std::string turn;           // full turn, for a first contact
    std::string turnCompact;    // short turn, for a continued conversation
    uint64_t    staticHash = 0;

    std::string Session() const { return staticPrompt + history; }
    std::string Full() const { return Session() + turn; }
};

BotPromptParts GenerateBotPromptParts(Player* bot, std::string playerMessage, Player* player);
std::string GenerateBotPrompt(Player* bot, std::string playerMessage, Player* player);

// "Right now: It is evening. You are on your mount ..." -- where the bot is and what it is doing.
std::string OllamaDescribeSituation(Player* bot, Player* player);
std::string BuildEmoteReactionPrompt(Player* bot, Player* player, uint32_t textEmote);

// Bounded, distance-sorted snapshot helpers used by the prompt builders and
// the topic engine.
std::string GenerateBotGameStateSnapshot(Player* bot);

class PlayerBotChatHandler : public PlayerScript
{
public:
    PlayerBotChatHandler() : PlayerScript("PlayerBotChatHandler", {
        PLAYERHOOK_CAN_PLAYER_USE_CHAT,
        PLAYERHOOK_CAN_PLAYER_USE_PRIVATE_CHAT,
        PLAYERHOOK_CAN_PLAYER_USE_GROUP_CHAT,
        PLAYERHOOK_CAN_PLAYER_USE_GUILD_CHAT,
        PLAYERHOOK_CAN_PLAYER_USE_CHANNEL_CHAT,
    }) {}

    bool OnPlayerCanUseChat(Player* player, uint32_t type, uint32_t lang, std::string& msg, Player* receiver) override;
    bool OnPlayerCanUseChat(Player* player, uint32_t type, uint32_t lang, std::string& msg) override;
    bool OnPlayerCanUseChat(Player* player, uint32_t type, uint32_t lang, std::string& msg, Group* group) override;
    bool OnPlayerCanUseChat(Player* player, uint32_t type, uint32_t lang, std::string& msg, Guild* guild) override;
    bool OnPlayerCanUseChat(Player* player, uint32_t type, uint32_t lang, std::string& msg, Channel* channel) override;

    static void ProcessChat(Player* player, uint32_t type, uint32_t lang, std::string& msg,
                            ChatChannelSourceLocal sourceLocal, Channel* channel = nullptr,
                            Player* receiver = nullptr, uint8_t chainDepth = 0);
};

// Housekeeping: prunes the per-bot maps that used to grow forever.
class OllamaChatMaintenance : public PlayerScript
{
public:
    OllamaChatMaintenance() : PlayerScript("OllamaChatMaintenance", { PLAYERHOOK_ON_LOGOUT, PLAYERHOOK_ON_DELETE_FROM_DB }) {}
    void OnPlayerLogout(Player* player) override;
    // A deleted character takes what the module stored about it along (Conquest of Azeroth).
    void OnPlayerDeleteFromDB(CharacterDatabaseTransaction trans, uint32 guid) override;
};

#endif // MOD_OLLAMA_CHAT_HANDLER_H
