#ifndef MOD_OLLAMA_CHAT_COA_H
#define MOD_OLLAMA_CHAT_COA_H

#include <cstdint>
#include <string>

// Conquest of Azeroth: the races and classes the way this world has them.
//
// mod-playerbots names only the ten stock races, so every custom race of CoA Custom (Worgen, Vulpera, Dracthyr,
// Sethrak, ...) reached the prompt as an empty word, and the bot had no idea what it was. Race and class names come
// from the server's own DBC tables here, and each race and class has a voice: how it speaks, what it cares about,
// where it comes from. Races are matched by name, because custom races get different numbers on different servers.

// The race's name as the game shows it ("High Elf", "Highmountain Tauren"); never empty.
std::string Coa_RaceName(uint8_t race);

// The class's name as the game shows it ("Felsworn", "Knight of Xoroth"); never empty.
std::string Coa_ClassName(uint8_t cls);

// How a member of this race speaks and what it carries with it; "" when the race is not known.
std::string Coa_RaceVoice(uint8_t race);

// What a member of this class notices and how it sees the world; "" when the class is not known.
std::string Coa_ClassVoice(uint8_t cls);

// The model sometimes answers with its own instructions instead of a line of speech. True when the reply carries
// phrases of the instructions, or several longer word runs of the prompt it was given.
bool Coa_LooksLikeEcho(const std::string& reply, const std::string& prompt);

// Where in the message this character is named, or npos. People do not type names the way the game stores them:
// "klaus" for Klausdieter, "klaus dieter", "Klaudieter". Counted are the whole name, a beginning of it of at least
// four letters, the name split in two words, and one typo in names of five letters or more. Common words ("there",
// "help", "doch") never count, so a bot called Thereon is not called by every "there".
size_t Coa_FindName(const std::string& message, const std::string& name);

// An action written into a reply ("*grins wickedly*", "*schaut angesengt an sich herunter*") as an emote line in
// the third person, without the name in front, which the game adds itself: "grins wickedly". The action is taken out
// of the reply. "" when the reply has none. Second person ("you look ...") is turned into third person where the
// verb allows it.
std::string Coa_TakeActionEmote(std::string& reply, const std::string& botName);

#endif
