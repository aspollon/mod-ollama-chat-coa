#ifndef MOD_OLLAMA_CHAT_PRESENCE_H
#define MOD_OLLAMA_CHAT_PRESENCE_H

#include <cstdint>
#include <string>

class Player;

// Conquest of Azeroth: a bot in a conversation stays with the person.
//
// While a person talks with a bot (it answered them, or they named or targeted it, within
// OllamaChat.Conversation.HoldSeconds), the bot's own errands wait: it stops, turns to the person and follows them a
// few steps when they walk on. A resident of Immersion stands at its place instead. It lets go when the
// conversation goes quiet, when the person says goodbye and the bot has answered, when the person walks far away,
// and always in combat, in dungeons, raids and battlegrounds, and for bots that travel in a group anyway.

void Presence_Update(uint32_t diff);                         // world thread, every tick
void Presence_NoteMessage(Player* player, const std::string& message);  // a person's line: goodbye?
void Presence_AfterReply(Player* bot, Player* player);       // the bot answered: was that the farewell?
bool Presence_IsHeld(Player* bot);                           // for CoaChatBridge::IsTalking
void Presence_RegisterBridge();

#endif
