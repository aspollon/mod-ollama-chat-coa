#ifndef MOD_OLLAMA_CHAT_BONDS_H
#define MOD_OLLAMA_CHAT_BONDS_H

#include <string>

class Player;

// What the modules of the bridge say about who the bot is right now; "" when none says anything.
std::string Bond_PersonaSection(Player* bot);

// Once, at startup: register the bridge's voice.
void Bond_RegisterBridge();

#endif
