#ifndef MOD_OLLAMA_CHAT_BONDS_H
#define MOD_OLLAMA_CHAT_BONDS_H

#include <cstdint>
#include <string>

class Creature;
class Player;
class Quest;

// Conquest of Azeroth: how close a bot is to the people it knows, and what they went through together.
//
// A bond runs from enemy to closest friend, and to lovers where both sides want it. It grows with friendly words
// and adventures shared, and sours with insults. The bot also knows when it last spoke with someone and what about,
// so a conversation can pick up where it left off the next day.
//
// Experiences come from the game itself (a hard fight, a quest, a fall, a duel, together with a person or alone)
// and from other modules through CoaChatBridge (a trade of mod-playerbots-auctions). The bridge's voice and persona
// parts are served here too.

enum class BondTone : uint8_t { Neutral, Friendly, Romantic, Hostile };

void Bond_Load();
void Bond_SaveAll();
void Bond_Update();                 // every few seconds, world thread: summaries of conversations that went quiet
void Bond_RegisterBridge();         // once, at startup
void Bond_Forget(uint64_t guid);    // a deleted character: its bonds, and the bonds of others with it

// A person and the bot exchanged a line (world thread).
void Bond_NoteTalk(Player* bot, Player* other);
// How the person's last message sounded (any thread).
void Bond_ApplyTone(uint64_t botGuid, uint64_t otherGuid, BondTone tone);
// Something shared changed the bond (world thread).
void Bond_NoteShared(Player* bot, Player* other, int8_t change);

// The bond section of a prompt: who `other` is to the bot, how to treat them, when they last spoke and what about.
std::string Bond_BuildPromptSection(Player* bot, Player* other);
// What the modules of the bridge say about who the bot is right now; "" when none says anything.
std::string Bond_PersonaSection(Player* bot);

// Experiences from the game (world thread).
void Experience_OnCreatureKill(Player* killer, Creature* victim);
void Experience_OnQuestComplete(Player* player, Quest const* quest);
void Experience_OnDeath(Player* player);
void Experience_OnDuelEnd(Player* winner, Player* loser);

#endif
