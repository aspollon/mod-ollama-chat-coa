#include "mod-ollama-chat_hygiene.h"

#include "DatabaseEnv.h"
#include "Field.h"
#include "QueryResult.h"
#include "Log.h"

#include <string>
#include <utility>
#include <vector>

namespace
{
    // Every table and column that holds a character guid.
    const std::vector<std::pair<const char*, const char*>>& GuidColumns()
    {
        static const std::vector<std::pair<const char*, const char*>> columns = {
            { "mod_ollama_chat_personality",           "guid" },
            { "mod_ollama_chat_history",               "bot_guid" },
            { "mod_ollama_chat_history",               "player_guid" },
            { "mod_ollama_chat_memories",              "bot_guid" },
            { "mod_ollama_chat_memories",              "about_guid" },
            { "mod_ollama_chat_relationships",         "bot_guid" },
            { "mod_ollama_chat_relationships",         "other_guid" },
            { "mod_ollama_chat_bot_player_sentiments", "bot_guid" },
            { "mod_ollama_chat_bot_player_sentiments", "player_guid" },
            { "mod_ollama_chat_bonds",                 "bot_guid" },
            { "mod_ollama_chat_bonds",                 "other_guid" },
        };
        return columns;
    }

    uint64_t Count(const std::string& sql)
    {
        if (QueryResult r = CharacterDatabase.Query(sql))
            return r->Fetch()[0].Get<uint64>();
        return 0;
    }
}

void Hygiene_Run()
{
    // Guids whose character is gone, or is not the one it was.
    const std::string changed =
        "SELECT i.guid FROM mod_ollama_chat_identity i LEFT JOIN characters c ON c.guid = i.guid "
        "WHERE c.guid IS NULL OR c.name <> i.name OR c.race <> i.race OR c.class <> i.class OR c.gender <> i.gender";

    const uint64_t replaced = Count("SELECT COUNT(*) FROM (" + changed + ") AS x");

    uint64_t removed = 0;
    for (auto const& [table, column] : GuidColumns())
    {
        const std::string where = std::string(column) + " <> 0 AND (" + column + " NOT IN (SELECT guid FROM characters) OR " +
                                  column + " IN (SELECT guid FROM (" + changed + ") AS gone))";
        removed += Count("SELECT COUNT(*) FROM " + std::string(table) + " WHERE " + where);
        CharacterDatabase.DirectExecute("DELETE FROM " + std::string(table) + " WHERE " + where);
    }

    CharacterDatabase.DirectExecute(
        "DELETE i FROM mod_ollama_chat_identity i LEFT JOIN characters c ON c.guid = i.guid "
        "WHERE c.guid IS NULL OR c.name <> i.name OR c.race <> i.race OR c.class <> i.class OR c.gender <> i.gender");

    // Remember who every guid with something stored belongs to now.
    std::string all;
    for (auto const& [table, column] : GuidColumns())
        all += std::string(all.empty() ? "" : " UNION ") + "SELECT " + column + " FROM " + table;
    CharacterDatabase.DirectExecute(
        "INSERT IGNORE INTO mod_ollama_chat_identity (guid, name, race, class, gender) "
        "SELECT c.guid, c.name, c.race, c.class, c.gender FROM characters c WHERE c.guid IN (" + all + ")");

    if (removed)
        LOG_INFO("module.ollamachat", "[Ollama Chat] Removed {} stored rows of deleted characters ({} guids now belong "
                 "to someone else).", removed, replaced);
}

void Hygiene_OnCharacterDeleted(CharacterDatabaseTransaction trans, uint32_t guid)
{
    for (auto const& [table, column] : GuidColumns())
        trans->Append("DELETE FROM " + std::string(table) + " WHERE " + column + " = " + std::to_string(guid));
    trans->Append("DELETE FROM mod_ollama_chat_identity WHERE guid = " + std::to_string(guid));
}
