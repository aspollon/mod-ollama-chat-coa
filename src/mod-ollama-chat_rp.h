#ifndef MOD_OLLAMA_CHAT_RP_H
#define MOD_OLLAMA_CHAT_RP_H

#include "ScriptMgr.h"

#include <cstdint>
#include <functional>
#include <string>

class Player;

// Roleplay sheets (client add-on AFKRoleplay): players write their own, the language model writes the bots'.
// Exchanged as hidden "AFKRP" addon messages; bots and the prompts read them. World thread only.
void Rp_LoadConfig();
void Rp_Update(uint32_t diff);
std::string Rp_PromptSection(Player* bot, Player* player);
// Forgets the sheets the language model wrote, for one bot by name or for all ("all"); they are written again when
// asked for or near a player. Players' own sheets stay. Returns how many were removed.
uint32_t Rp_ResetBotSheets(std::string const& name);
// A bot spoken to without a sheet writes it first: true when `then` waits for it (runs when the sheet is written,
// failed, or after OllamaChat.RP.SheetFirstWaitSeconds). False: answer now.
bool Rp_SheetFirst(Player* bot, std::function<void()> then);

class OllamaRpChat : public PlayerScript
{
public:
    OllamaRpChat() : PlayerScript("OllamaRpChat", { PLAYERHOOK_ON_BEFORE_SEND_CHAT_MESSAGE }) {}
    void OnPlayerBeforeSendChatMessage(Player* player, uint32& type, uint32& lang, std::string& msg) override;
};

#endif
