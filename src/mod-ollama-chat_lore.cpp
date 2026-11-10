#include "mod-ollama-chat_lore.h"
#include "mod-ollama-chat_coa.h"
#include "mod-ollama-chat_config.h"

#include "Config.h"
#include "Player.h"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <unordered_map>
#include <vector>

namespace
{
    std::string Lower(std::string v)
    {
        for (char& c : v)
            c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        return v;
    }

    bool WordByte(unsigned char c)
    {
        return std::isalnum(c) || c == '\'' || c == '-' || (c & 0x80);
    }

    // `phrase` in `text` as whole words.
    bool HasPhrase(const std::string& text, const std::string& phrase)
    {
        if (phrase.empty())
            return false;
        for (size_t pos = text.find(phrase); pos != std::string::npos; pos = text.find(phrase, pos + 1))
        {
            const bool startOk = pos == 0 || !WordByte(static_cast<unsigned char>(text[pos - 1]));
            const size_t end = pos + phrase.size();
            // A plural or possessive still counts: "worgens", "Arthas's".
            size_t after = end;
            if (after < text.size() && text[after] == 's')
                ++after;
            else if (after + 1 < text.size() && text[after] == '\'' && text[after + 1] == 's')
                after += 2;
            const bool endOk = end >= text.size() || !WordByte(static_cast<unsigned char>(text[end])) ||
                               after >= text.size() || !WordByte(static_cast<unsigned char>(text[after]));
            if (startOk && endOk)
                return true;
        }
        return false;
    }

    // Score of an entry for a message: the length of the longest keyword found (longer phrases are surer hits).
    size_t Score(const std::string& lowerMessage, const LoreEntry& entry)
    {
        size_t best = 0;
        std::string kw;
        for (const char* p = entry.keywords; ; ++p)
        {
            if (*p == ',' || *p == '\0')
            {
                if (kw.size() >= 3 && kw.size() > best && HasPhrase(lowerMessage, kw))
                    best = kw.size();
                kw.clear();
                if (*p == '\0')
                    break;
            }
            else
                kw += *p;
        }
        return best;
    }

    const LoreEntry* RaceEntry(uint8_t race)
    {
        static std::unordered_map<std::string, const LoreEntry*> byName;
        if (byName.empty())
            for (size_t i = 0; i < LoreData::RaceCount; ++i)
                byName[LoreData::Races[i].key] = &LoreData::Races[i];
        auto it = byName.find(Lower(Coa_RaceName(race)));
        return it == byName.end() ? nullptr : it->second;
    }

    const LoreEntry* ById(const LoreEntry* table, size_t count, uint32_t id)
    {
        for (size_t i = 0; i < count; ++i)
            if (table[i].id == id)
                return &table[i];
        return nullptr;
    }

    std::string FirstSentence(const char* text)
    {
        std::string t = text;
        size_t dot = t.find(". ");
        return dot == std::string::npos ? t : t.substr(0, dot + 1);
    }

    // "30-43" -> 30; the default when the option is missing or unreadable.
    uint32_t RangeStart(const char* option, uint32_t fallback)
    {
        std::string v = sConfigMgr->GetOption<std::string>(option, "");
        unsigned a = 0, b = 0;
        if (std::sscanf(v.c_str(), "%u-%u", &a, &b) >= 1 && a > 0)
            return a;
        return fallback;
    }

    // Where the bot stands on the road through the world, in words. With mod-world-journey the old world, Outland
    // and Northrend follow one another on the way to 60; without it, the original levels hold.
    std::string Road(Player* bot)
    {
        const bool journey = sConfigMgr->GetOption<bool>("Journey.Enable", false);
        const uint32_t outland   = journey ? RangeStart("Journey.Outland.Range", 30) : 58;
        const uint32_t northrend = journey ? RangeStart("Journey.Northrend.Range", 40) : 68;
        const uint32_t endgame   = journey ? sConfigMgr->GetOption<uint32_t>("Journey.Endgame.Level", 60) : 80;
        const uint32_t level = bot->GetLevel();

        std::string road;
        if (level >= endgame)
            road = "You are a veteran: the old world, Outland and Northrend lie behind you, and the great raids and "
                   "heroic dungeons are what is left to test you.";
        else if (level >= northrend)
            road = "You are seasoned enough to fight in Northrend, where the war against the Lich King is fought.";
        else if (level >= outland)
            road = "You are seasoned enough to cross the Dark Portal and fight in Outland; Northrend is still beyond "
                   "you.";
        else if (level >= 10)
            road = "Your road still runs through the old lands of Azeroth; Outland and Northrend are beyond you yet.";
        else
            road = "You are new to adventuring and still learning near home.";

        const uint32_t map = bot->GetMapId();
        if ((map == 530 && level < outland && bot->GetZoneId() != 3430 && bot->GetZoneId() != 3433 &&
             bot->GetZoneId() != 3524 && bot->GetZoneId() != 3525 && bot->GetZoneId() != 3487 &&
             bot->GetZoneId() != 3557) ||
            (map == 571 && level < northrend))
            road += " The land you are in now is far too dangerous for you.";
        return road + " Never speak of levels; say it the way a person of the world would.";
    }
}

std::string Lore_BuildSection(Player* bot, Player* other, const std::string& message)
{
    if (!bot || !g_LoreEnable)
        return "";

    std::vector<const LoreEntry*> used;
    std::string out = " What you know of the world (use it only when it fits, speak of it as someone who lived in "
                      "it, never recite it):";

    if (const LoreEntry* race = RaceEntry(bot->getRace()))
    {
        out += " Your people: " + std::string(race->text);
        used.push_back(race);
    }
    if (const LoreEntry* cls = ById(LoreData::Classes, LoreData::ClassCount, bot->getClass()))
    {
        out += " Your calling: " + std::string(cls->text);
        used.push_back(cls);
    }
    if (const LoreEntry* zone = ById(LoreData::Zones, LoreData::ZoneCount, bot->GetZoneId()))
    {
        out += " This place, " + std::string(zone->key) + ": " + zone->text;
        used.push_back(zone);
    }
    out += " " + Road(bot);

    if (other && other != bot && other->getRace() != bot->getRace())
        if (const LoreEntry* theirs = RaceEntry(other->getRace()))
        {
            out += " About " + std::string(other->GetName()) + "'s people, the " + Coa_RaceName(other->getRace()) +
                   ": " + FirstSentence(theirs->text);
            used.push_back(theirs);
        }

    // What the message names.
    if (!message.empty() && g_LoreMaxTopics > 0)
    {
        const std::string lowerMessage = Lower(message);
        std::vector<std::pair<size_t, const LoreEntry*>> hits;
        auto scan = [&](const LoreEntry* table, size_t count)
        {
            for (size_t i = 0; i < count; ++i)
            {
                const LoreEntry* e = &table[i];
                if (std::find(used.begin(), used.end(), e) != used.end())
                    continue;
                if (size_t s = Score(lowerMessage, *e))
                    hits.emplace_back(s, e);
            }
        };
        scan(LoreData::Topics, LoreData::TopicCount);
        scan(LoreData::Zones, LoreData::ZoneCount);
        scan(LoreData::Races, LoreData::RaceCount);
        scan(LoreData::Classes, LoreData::ClassCount);
        std::stable_sort(hits.begin(), hits.end(),
                         [](const auto& a, const auto& b) { return a.first > b.first; });
        for (size_t i = 0; i < hits.size() && i < g_LoreMaxTopics; ++i)
            out += std::string(" You also know: ") + hits[i].second->text;
    }
    return out;
}
