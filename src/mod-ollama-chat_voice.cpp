#include "mod-ollama-chat_voice.h"
#include "mod-ollama-chat_config.h"

#include "CellImpl.h"
#include "Chat.h"
#include "GridNotifiers.h"
#include "GridNotifiersImpl.h"
#include "Group.h"
#include "ObjectAccessor.h"
#include "Player.h"
#include "WorldPacket.h"
#include "WorldSession.h"

#include <list>
#include <unordered_map>

// Wire format of github.com/Crono141/CompanionVoice (its server/mod-companion-voice does the same from a DB table).
namespace
{
    char const* const PREFIX = "CVOX";

    uint32 Fnv1a(std::string const& s)   // the add-on hashes the line as it arrives
    {
        uint32 h = 2166136261u;
        for (unsigned char c : s) { h ^= c; h *= 16777619u; }
        return h;
    }

    bool Hears(Player* p)
    {
        return p && p->IsInWorld() && p->GetSession() && !p->GetSession()->IsBot();
    }

    void Send(Player* from, Player* to, std::string const& body)
    {
        WorldPacket data;
        ChatHandler::BuildChatPacket(data, CHAT_MSG_WHISPER, LANG_ADDON, from, to, std::string(PREFIX) + "\t" + body);
        to->SendDirectMessage(&data);
    }

    void InRange(Player* bot, float distance, std::list<Player*>& found)
    {
        Acore::AnyPlayerInObjectRangeCheck check(bot, distance, false, true);
        Acore::PlayerListSearcher<Acore::AnyPlayerInObjectRangeCheck> searcher(bot, found, check);
        Cell::VisitObjects(bot, searcher, distance);
    }

    std::unordered_map<uint32, std::pair<std::string, uint32>> g_rosters;   // player -> (bot names, ms since sent)
    uint32 g_rosterTimer = 0;
}

void Voice_Label(Player* bot, Player* whisperTo, OllamaVoiceScope scope, std::string const& text)
{
    if (!g_VoiceLabels || !bot || text.empty())
        return;
    std::string const body = "neutral\t" + std::to_string(Fnv1a(text));
    switch (scope)
    {
        case VOICE_WHISPER:
            if (whisperTo != bot && Hears(whisperTo))
                Send(bot, whisperTo, body);
            break;
        case VOICE_PARTY:
            if (Group* group = bot->GetGroup())
                for (GroupReference* ref = group->GetFirstMember(); ref; ref = ref->next())
                    if (Player* m = ref->GetSource(); m != bot && Hears(m))
                        Send(bot, m, body);
            break;
        case VOICE_SAY:
        case VOICE_YELL:
        {
            std::list<Player*> found;
            InRange(bot, scope == VOICE_SAY ? g_SayDistance : g_YellDistance, found);
            for (Player* p : found)
                if (p != bot && Hears(p))
                    Send(bot, p, body);
            break;
        }
    }
}

// the bots in each real player's group, on change and every minute: their unlabelled lines (stock status text) stay quiet
void Voice_Update(uint32_t diff)
{
    if (!g_VoiceLabels)
        return;
    g_rosterTimer += diff;
    if (g_rosterTimer < 5000)
        return;
    uint32 const elapsed = g_rosterTimer;
    g_rosterTimer = 0;

    std::unordered_map<uint32, std::pair<std::string, uint32>> next;
    for (auto const& [guid, player] : ObjectAccessor::GetPlayers())
    {
        if (!Hears(player))
            continue;
        Group* group = player->GetGroup();
        if (!group)
            continue;
        std::string names;
        for (GroupReference* ref = group->GetFirstMember(); ref; ref = ref->next())
        {
            Player* m = ref->GetSource();
            if (m && m != player && m->GetSession() && m->GetSession()->IsBot())
                names += (names.empty() ? "" : ",") + m->GetName();
        }
        if (names.empty())
            continue;
        uint32 const key = player->GetGUID().GetCounter();
        auto old = g_rosters.find(key);
        uint32 since = old == g_rosters.end() ? 0 : old->second.second + elapsed;
        if (old == g_rosters.end() || old->second.first != names || since >= 60000)
        {
            Send(player, player, "bots\t" + names.substr(0, 200));
            since = 0;
        }
        next[key] = {names, since};
    }
    g_rosters.swap(next);
}
