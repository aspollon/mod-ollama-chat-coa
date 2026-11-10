#ifndef MOD_OLLAMA_CHAT_HYGIENE_H
#define MOD_OLLAMA_CHAT_HYGIENE_H

#include "DatabaseEnvFwd.h"
#include <cstdint>

// Conquest of Azeroth: what the module stored about characters that no longer exist goes with them.
//
// At start, before anything is loaded: conversation history, memories, relationships, moods, bonds and temperaments
// of deleted characters are removed - on both sides, the bot's own and what bots keep about a deleted person - and so
// is everything stored under a guid that now belongs to a different character (a bot reset deletes bots straight from
// the database, and the server can hand their guids to new characters). A character deleted while the server runs is
// cleaned up at once.
void Hygiene_Run();
void Hygiene_OnCharacterDeleted(CharacterDatabaseTransaction trans, uint32_t guid);

#endif
