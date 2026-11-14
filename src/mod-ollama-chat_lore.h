#ifndef MOD_OLLAMA_CHAT_LORE_H
#define MOD_OLLAMA_CHAT_LORE_H

#include <cstddef>
#include <cstdint>
#include <string>

class Player;

// What the bots know of Azeroth (Conquest of Azeroth edition).
//
// Every bot knows the story of its own people, the order or tradition of its calling and the place it stands in,
// and something of the people of whoever it talks to. A message naming a person, a place, a people, an order or an
// event ("Arthas", "Gilneas", "the Scourge", "Wintergrasp") brings up what the bot knows about it. Where the server
// runs mod-world-journey, the bot also knows how far along the road from the old world to Outland and Northrend it is,
// in words, never in levels.
//
// The knowledge is compiled into the module (mod-ollama-chat_lore_data.cpp), so there is no data folder to find.

struct LoreEntry
{
    const char* key;        // race: lower-case race name; zone: zone name; topic: its name
    uint32_t    id;         // class id or zone id; 0 otherwise
    const char* keywords;   // lower-case phrases, comma separated
    const char* text;
};

namespace LoreData
{
    extern const LoreEntry Races[];
    extern const size_t    RaceCount;
    extern const LoreEntry Classes[];
    extern const size_t    ClassCount;
    extern const LoreEntry Zones[];
    extern const size_t    ZoneCount;
    extern const LoreEntry Topics[];
    extern const size_t    TopicCount;
}

// The knowledge section of a prompt: the bot's own people, calling, place and road, the people of `other` (may be
// nullptr), and up to OllamaChat.Lore.MaxTopics entries named in `message`. "" when OllamaChat.Lore.Enable is 0.
std::string Lore_BuildSection(Player* bot, Player* other, const std::string& message);

// The same knowledge in two parts. StaticSection is everything that does not depend on the message: the bot's
// people, calling, place, road and the other person's people. TopicSection is what the current message names.
// A conversation context keeps the static part and only the topic part is sent again on later turns.
std::string Lore_BuildStaticSection(Player* bot, Player* other);
std::string Lore_BuildTopicSection(Player* bot, Player* other, const std::string& message);

#endif
