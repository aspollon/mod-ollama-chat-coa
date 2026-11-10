#include "mod-ollama-chat_presence.h"
#include "CoaChatBridge.h"
#include "mod-ollama-chat_config.h"
#include "mod-ollama-chat_governor.h"
#include "mod-ollama-chat_world.h"

#include "Group.h"
#include "Map.h"
#include "MotionMaster.h"
#include "ObjectAccessor.h"
#include "Player.h"

#include "PlayerbotAI.h"
#include "PlayerbotMgr.h"

#include <algorithm>
#include <cctype>
#include <ctime>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace
{
    std::unordered_set<uint64_t> g_held;                    // bots held last pass
    std::unordered_map<uint64_t, time_t> g_farewell;        // person -> when they said goodbye

    std::string Lower(std::string v)
    {
        for (char& c : v)
            c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        return v;
    }

    bool HasPhrase(const std::string& text, const std::string& phrase)
    {
        for (size_t pos = text.find(phrase); pos != std::string::npos; pos = text.find(phrase, pos + 1))
        {
            const bool startOk = pos == 0 || !std::isalnum(static_cast<unsigned char>(text[pos - 1]));
            const size_t end = pos + phrase.size();
            const bool endOk = end >= text.size() || !std::isalnum(static_cast<unsigned char>(text[end]));
            if (startOk && endOk)
                return true;
        }
        return false;
    }

    bool IsFarewell(const std::string& message)
    {
        static const char* const phrases[] = {
            "bye", "goodbye", "good bye", "bye bye", "farewell", "see you", "see ya", "cya", "gotta go", "got to go",
            "have to go", "take care", "good night", "i'm off", "im off", "so long",
            "tschüss", "tschüs", "tschuess", "tschau", "ciao", "bis dann", "bis später", "bis spaeter", "bis bald",
            "auf wiedersehen", "mach's gut", "machs gut", "gute nacht", "leb wohl", "ich muss los", "muss los",
        };
        const std::string lower = Lower(message);
        for (const char* p : phrases)
            if (HasPhrase(lower, p))
                return true;
        return false;
    }

    bool Anchored(Player* bot)
    {
        for (auto const& fn : CoaChatBridge::Anchors())
            if (fn && fn(bot))
                return true;
        return false;
    }

    // Whether this bot may be held for this person now.
    bool CanHold(Player* bot, Player* player, double idle)
    {
        if (!bot || !player || !bot->IsInWorld() || !player->IsInWorld())
            return false;
        if (idle > double(g_ConversationHoldSeconds) || !OllamaIsBotPlayer(bot) || OllamaIsBotPlayer(player))
            return false;
        if (!bot->IsAlive() || !player->IsAlive() || bot->IsInCombat() || player->IsInCombat())
            return false;
        if (bot->GetMapId() != player->GetMapId() || bot->IsBeingTeleported() || bot->IsInFlight())
            return false;
        if (Map* map = bot->GetMap())
            if (map->Instanceable())
                return false;   // dungeons, raids, battlegrounds, arenas
        if (bot->duel || player->duel)
            return false;
        if (bot->GetDistance(player) > 45.0f)
            return false;   // the person walked off: the conversation is over
        // A bot travelling in the person's group, or with a master of its own, already has its orders.
        if (Group* group = bot->GetGroup())
            if (group->IsMember(player->GetGUID()))
                return false;
        if (PlayerbotAI* ai = PlayerbotsMgr::instance().GetPlayerbotAI(bot))
        {
            Player* master = ai->GetMaster();
            if (master && master != bot && OllamaIsRealPlayer(master))
                return false;
        }
        else
            return false;
        return true;
    }

    void Release(Player* bot)
    {
        if (!bot)
            return;
        if (bot->GetMotionMaster()->GetCurrentMovementGeneratorType() == FOLLOW_MOTION_TYPE)
            bot->GetMotionMaster()->Clear(false);
        if (PlayerbotAI* ai = PlayerbotsMgr::instance().GetPlayerbotAI(bot))
            ai->SetNextCheckDelay(0);
    }
}

void Presence_NoteMessage(Player* player, const std::string& message)
{
    if (!player || !g_ConversationHoldSeconds)
        return;
    if (IsFarewell(message))
        g_farewell[player->GetGUID().GetRawValue()] = time(nullptr);
}

void Presence_AfterReply(Player* bot, Player* player)
{
    if (!bot || !player)
        return;
    auto it = g_farewell.find(player->GetGUID().GetRawValue());
    if (it == g_farewell.end())
        return;
    // The bot has said goodbye too: the conversation is over and it goes about its business.
    g_farewell.erase(it);
    if (Governor_GetPartner(player->GetGUID()) == bot->GetGUID())
        Governor_EndConversation(player->GetGUID());
}

bool Presence_IsHeld(Player* bot)
{
    return bot && g_held.count(bot->GetGUID().GetRawValue()) != 0;
}

void Presence_RegisterBridge()
{
    static bool registered = false;
    if (registered)
        return;
    registered = true;
    CoaChatBridge::Talking() = Presence_IsHeld;
}

void Presence_Update(uint32_t diff)
{
    static uint32_t timer = 0;
    if (timer > diff)
    {
        timer -= diff;
        return;
    }
    timer = 1000;

    const time_t now = time(nullptr);

    // A goodbye nobody answered still ends the conversation after a while.
    for (auto it = g_farewell.begin(); it != g_farewell.end();)
    {
        if (it->second + 30 < now)
        {
            Governor_EndConversation(ObjectGuid(it->first));
            it = g_farewell.erase(it);
        }
        else
            ++it;
    }

    std::unordered_set<uint64_t> held;
    if (g_Enable && g_ConversationHoldSeconds)
    {
        for (const GovernorPartner& p : Governor_Partners())
        {
            Player* player = ObjectAccessor::FindConnectedPlayer(p.player);
            Player* bot = ObjectAccessor::FindConnectedPlayer(p.bot);
            if (!CanHold(bot, player, p.idleSeconds))
                continue;
            PlayerbotAI* ai = PlayerbotsMgr::instance().GetPlayerbotAI(bot);
            const uint64_t key = bot->GetGUID().GetRawValue();
            held.insert(key);

            // Its own plans wait: the AI does nothing while the delay runs, and it is renewed every second.
            ai->SetNextCheckDelay(2500);

            const bool fresh = !g_held.count(key);
            const bool anchored = Anchored(bot);
            MotionMaster* motion = bot->GetMotionMaster();
            if (fresh)
            {
                bot->StopMoving();
                motion->Clear(false);
            }

            const float distance = bot->GetDistance(player);
            if (!anchored && distance > 6.0f && motion->GetCurrentMovementGeneratorType() != FOLLOW_MOTION_TYPE)
                motion->MoveFollow(player, 2.5f, float(M_PI) / 2.0f);   // at their side
            else if (!bot->isMoving())
                bot->SetFacingToObject(player);
        }
    }

    // Let go of the bots no longer held.
    for (uint64_t key : g_held)
        if (!held.count(key))
            Release(ObjectAccessor::FindConnectedPlayer(ObjectGuid(key)));
    g_held.swap(held);
}
