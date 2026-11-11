#ifndef MOD_OLLAMA_CHAT_RP_H
#define MOD_OLLAMA_CHAT_RP_H

#include "ScriptMgr.h"

#include <cstdint>
#include <string>

class Player;

// Roleplay sheets (client add-on AFKRoleplay): players write their own, the language model writes the bots'.
// Exchanged as hidden "AFKRP" addon messages; bots and the prompts read them. World thread only.
void Rp_LoadConfig();
void Rp_Update(uint32_t diff);
std::string Rp_PromptSection(Player* bot, Player* player);

class OllamaRpChat : public PlayerScript
{
public:
    OllamaRpChat() : PlayerScript("OllamaRpChat", { PLAYERHOOK_ON_BEFORE_SEND_CHAT_MESSAGE }) {}
    void OnPlayerBeforeSendChatMessage(Player* player, uint32& type, uint32& lang, std::string& msg) override;
};

#endif
