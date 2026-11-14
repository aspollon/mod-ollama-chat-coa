#include "mod-ollama-chat_bonds.h"
#include "CoaChatBridge.h"
#include "mod-ollama-chat_coa.h"
#include "mod-ollama-chat_config.h"
#include "mod-ollama-chat_dispatch.h"
#include "mod-ollama-chat_governor.h"
#include "mod-ollama-chat_handler.h"
#include "mod-ollama-chat_personality.h"
#include "mod-ollama-chat_roleplay.h"
#include "mod-ollama-chat_world.h"

#include "Player.h"
#include "PlayerbotAI.h"
#include "PlayerbotMgr.h"

#include <string>
#include <vector>

namespace
{
    bool IsBot(Player* p)
    {
        return p && OllamaIsBotPlayer(p);
    }

    // ---- the bridge's voice ------------------------------------------------

    bool Voice(Player* bot, Player* to, std::string const& line, uint8_t channel, uint8_t mode,
               std::string const& context)
    {
        if (!g_Enable || !g_VoiceEnable || !bot || !bot->IsInWorld() || line.empty() || !IsBot(bot))
            return false;
        if (channel == CoaChatBridge::CHANNEL_WHISPER && !to)
            return false;

        PlayerbotAI* botAI = PlayerbotsMgr::instance().GetPlayerbotAI(bot);
        if (!botAI || !botAI->GetChatHelper())
            return false;

        std::string marked = line;
        std::vector<std::string> links = Coa_TakeLinks(marked);

        const std::string name = bot->GetName();
        std::string prompt = "You are " + name + ", a level " + std::to_string(bot->GetLevel()) + " " +
            Coa_RaceName(bot->getRace()) + " " + botAI->GetChatHelper()->FormatClass(bot->getClass()) + ". " +
            "Your temperament: " + GetPersonalityPromptAddition(GetBotPersonality(bot)) + " " +
            OllamaDescribeSituation(bot, to);
        prompt += Roleplay_BuildVoicePrompt(bot);
        if (!context.empty())
            prompt += " " + context;

        if (mode == CoaChatBridge::MODE_EXACT)
        {
            prompt += std::string(" Say the following") + (to ? " to " + std::string(to->GetName()) : "") +
                " in your own words, the way you would say it, in one or two short sentences. Keep its meaning, "
                "and keep every number, price, amount, item name and place exactly as it is";
            if (!links.empty())
                prompt += "; keep the markers [1], [2] and so on exactly as they are, they stand for items";
            prompt += ": \"" + marked + "\"";
        }
        else
            prompt += std::string(" Say something") + (to ? " to " + std::string(to->GetName()) : " to the people around you") +
                " in the spirit of this line, but in your own words and fitting the moment: \"" + marked + "\"";

        OllamaChatRequest request;
        request.botGuid   = bot->GetGUID().GetRawValue();
        request.targetGuid = to ? to->GetGUID().GetRawValue() : 0;
        request.source    = channel == CoaChatBridge::CHANNEL_WHISPER ? SRC_WHISPER_LOCAL
                          : channel == CoaChatBridge::CHANNEL_YELL    ? SRC_YELL_LOCAL
                                                                      : SRC_SAY_LOCAL;
        request.scopeKey  = Governor_MakeScopeKey(ChatChannelSourceLocalStr[request.source], 0, std::string(), 0,
                                                  bot->GetZoneId());
        request.directAddress    = true;
        request.prompt           = std::move(prompt);
        request.botName          = name;
        request.kind             = OllamaRequestKind::RoleplayReply;
        request.triggerBotReplies = false;
        request.voice            = true;
        request.voiceExact       = mode == CoaChatBridge::MODE_EXACT;
        request.voiceOriginal    = line;
        request.voiceLinks       = std::move(links);
        return OllamaDispatch_Submit(std::move(request));
    }
}

std::string Bond_PersonaSection(Player* bot)
{
    std::string out;
    for (auto const& fn : CoaChatBridge::Personas())
    {
        if (!fn)
            continue;
        std::string s = fn(bot);
        if (!s.empty())
            out += (out.empty() ? "" : " ") + s;
    }
    return out;
}

void Bond_RegisterBridge()
{
    static bool registered = false;
    if (registered)
        return;
    registered = true;
    CoaChatBridge::Voice() = Voice;
}
