#include "mod-ollama-chat_command.h"
#include "mod-ollama-chat_config.h"
#include "mod-ollama-chat_personality.h"
#include "mod-ollama-chat_api.h"
#include "mod-ollama-chat_capability.h"
#include "mod-ollama-chat_dispatch.h"
#include "mod-ollama-chat_governor.h"
#include "mod-ollama-chat_response.h"
#include "mod-ollama-chat_roleplay.h"
#include "mod-ollama-chat-utilities.h"
#include "Log.h"
#include "DatabaseEnv.h"
#include <thread>
#include "Chat.h"
#include "Config.h"
#include "ObjectAccessor.h"
#include "Player.h"
#include "PlayerbotMgr.h"
#include <fmt/core.h>

using namespace Acore::ChatCommands;

OllamaChatConfigCommand::OllamaChatConfigCommand()
    : CommandScript("OllamaChatConfigCommand")
{
}

ChatCommandTable OllamaChatConfigCommand::GetCommands() const
{
    static ChatCommandTable ollamaPersonalityCommandTable =
    {
        { "get",  HandleOllamaPersonalityGetCommand,  SEC_ADMINISTRATOR, Console::Yes },
        { "set",  HandleOllamaPersonalitySetCommand,  SEC_ADMINISTRATOR, Console::Yes },
        { "list", HandleOllamaPersonalityListCommand, SEC_ADMINISTRATOR, Console::Yes }
    };

    static ChatCommandTable ollamaReloadCommandTable =
    {
        { "reload",      HandleOllamaReloadCommand,  SEC_ADMINISTRATOR, Console::Yes },
        { "status",      HandleOllamaStatusCommand,  SEC_ADMINISTRATOR, Console::Yes },
        { "test",        HandleOllamaTestCommand,    SEC_ADMINISTRATOR, Console::Yes },
        { "personality", ollamaPersonalityCommandTable }
    };

    static ChatCommandTable commandTable =
    {
        { "ollama", ollamaReloadCommandTable }
    };

    return commandTable;
}

bool OllamaChatConfigCommand::HandleOllamaReloadCommand(ChatHandler* handler)
{
    sConfigMgr->Reload();
    LoadOllamaChatConfig();
    Roleplay_Load();

    // Re-probe: the operator may have just pointed the module at a different
    // model, and think-mode support is per-model.
    OllamaCapability_Init(true);

    // Clear personality assignments if RP personalities are disabled
    // This ensures that when re-enabled later, bots get fresh random assignments
    if (!g_EnableRPPersonalities)
    {
        ClearAllBotPersonalities();
    }

    LoadBotPersonalityList();
    LoadBotConversationHistoryFromDB();
    handler->SendSysMessage("OllamaChat: Configuration reloaded from conf!");
    return true;
}

bool OllamaChatConfigCommand::HandleOllamaPersonalityGetCommand(ChatHandler* handler, std::string botName)
{
    Player* bot = ObjectAccessor::FindPlayerByName(botName);
    if (!bot)
    {
        handler->SendSysMessage(fmt::format("OllamaChat: Bot '{}' not found.", botName));
        return true;
    }
    
    if (!PlayerbotsMgr::instance().GetPlayerbotAI(bot))
    {
        handler->SendSysMessage(fmt::format("OllamaChat: Player '{}' is not a bot.", botName));
        return true;
    }
    
    std::string personality = GetBotPersonality(bot);
    std::string prompt = GetPersonalityPromptAddition(personality);
    
    handler->SendSysMessage(fmt::format("OllamaChat: Bot '{}' has personality '{}'", botName, personality));
    handler->SendSysMessage(fmt::format("  Prompt: {}", prompt));
    
    return true;
}

bool OllamaChatConfigCommand::HandleOllamaPersonalitySetCommand(ChatHandler* handler, std::string botName, std::string personality)
{
    Player* bot = ObjectAccessor::FindPlayerByName(botName);
    if (!bot)
    {
        handler->SendSysMessage(fmt::format("OllamaChat: Bot '{}' not found.", botName));
        return true;
    }
    
    if (!PlayerbotsMgr::instance().GetPlayerbotAI(bot))
    {
        handler->SendSysMessage(fmt::format("OllamaChat: Player '{}' is not a bot.", botName));
        return true;
    }
    
    if (!PersonalityExists(personality))
    {
        handler->SendSysMessage(fmt::format("OllamaChat: Personality '{}' does not exist. Use '.ollama personality list' to see available personalities.", personality));
        return true;
    }
    
    if (SetBotPersonality(bot, personality))
    {
        std::string prompt = GetPersonalityPromptAddition(personality);
        handler->SendSysMessage(fmt::format("OllamaChat: Set bot '{}' personality to '{}'", botName, personality));
        handler->SendSysMessage(fmt::format("  Prompt: {}", prompt));
    }
    else
    {
        handler->SendSysMessage(fmt::format("OllamaChat: Failed to set personality for bot '{}'.", botName));
    }
    
    return true;
}

bool OllamaChatConfigCommand::HandleOllamaPersonalityListCommand(ChatHandler* handler)
{
    std::vector<std::string> personalities = GetAllPersonalityKeys();
    
    if (personalities.empty())
    {
        handler->SendSysMessage("OllamaChat: No personalities loaded.");
        return true;
    }
    
    handler->SendSysMessage(fmt::format("OllamaChat: Available personalities ({} total, {} random-assignable):", 
                            personalities.size(), g_PersonalityKeysRandomOnly.size()));
    
    for (const auto& personality : personalities)
    {
        std::string prompt = GetPersonalityPromptAddition(personality);
        
        // Check if this personality is manual-only
        bool isManualOnly = (std::find(g_PersonalityKeysRandomOnly.begin(), g_PersonalityKeysRandomOnly.end(), personality) 
                            == g_PersonalityKeysRandomOnly.end());
        
        std::string manualTag = isManualOnly ? " [MANUAL ONLY]" : "";
        
        handler->SendSysMessage(fmt::format("  - {}{}", personality, manualTag));
        handler->SendSysMessage(fmt::format("    {}", prompt));
    }
    
    return true;
}


// ---------------------------------------------------------------------------
// Diagnostics
// ---------------------------------------------------------------------------

bool OllamaChatConfigCommand::HandleOllamaStatusCommand(ChatHandler* handler)
{
    const OllamaDispatchStats dispatch = OllamaDispatch_GetStats();
    const GovernorStats       gov      = Governor_GetStats();

    handler->PSendSysMessage("|cff00ff00[Ollama Chat] Status|r");
    // The API key is deliberately not shown here.
    handler->PSendSysMessage("Module: {}   Provider: {}   Endpoint: {}   Model: {}   ApiKey: {}   HTTPS: {}",
                             g_Enable ? "enabled" : "DISABLED",
                             OllamaProvider_Name(g_OllamaProvider),
                             g_OllamaUrl, g_OllamaModel,
                             g_OllamaApiKey.empty() ? "not set" : "set",
                             OllamaHttp_TlsAvailable() ? "available" : "NOT BUILT (no OpenSSL)");
    handler->PSendSysMessage("Think: {}", OllamaCapability_StatusText());

    handler->PSendSysMessage("Dispatcher: {} workers, {} queued, {} in flight, {} awaiting delivery",
                             dispatch.workers, dispatch.queuedRequests,
                             dispatch.inFlight, dispatch.pendingDeliveries);
    handler->PSendSysMessage("Totals: {} submitted, {} delivered, {} failed",
                             (unsigned long long)dispatch.totalSubmitted,
                             (unsigned long long)dispatch.totalDelivered,
                             (unsigned long long)dispatch.totalFailed);
    handler->PSendSysMessage("Dropped: {} queue-full, {} empty-after-cleanup, {} by governor",
                             (unsigned long long)dispatch.totalDroppedQueueFull,
                             (unsigned long long)dispatch.totalDroppedEmpty,
                             (unsigned long long)dispatch.totalDroppedGovernor);

    handler->PSendSysMessage("Governor: {} bots, {} scopes tracked, {} sends in the last minute",
                             gov.trackedBots, gov.trackedScopes, gov.sendsLastMinute);
    handler->PSendSysMessage("Blocked: {} cooldown, {} rate, {} repetition, {} chain-depth, {} no-audience",
                             gov.blockedCooldown, gov.blockedRate, gov.blockedRepetition,
                             gov.blockedChainDepth, gov.blockedNoAudience);

    handler->PSendSysMessage("Roleplay: {} (strictness {})   Emote reactions: {}",
                             g_RoleplayEnable ? "on" : "off",
                             (uint32)g_RoleplayStrictness,
                             g_EnableEmoteReactions ? "on" : "off");
    handler->PSendSysMessage("Topic weights: people {} / world {} / activity {} / self {} / guild {}",
                             g_TopicWeightPeople, g_TopicWeightWorld, g_TopicWeightActivity,
                             g_TopicWeightSelf, g_TopicWeightGuild);

    if (!dispatch.lastError.empty())
        handler->PSendSysMessage("|cffff0000Last error:|r {}", dispatch.lastError);
    else
        handler->PSendSysMessage("Last error: none");

    return true;
}

bool OllamaChatConfigCommand::HandleOllamaTestCommand(ChatHandler* handler, Acore::ChatCommands::Tail prompt)
{
    std::string text(prompt);
    if (text.empty())
    {
        handler->SendSysMessage("Usage: .ollama test <prompt>");
        handler->SetSentErrorMessage(true);
        return false;
    }

    handler->PSendSysMessage("[Ollama Chat] Sending test prompt, please wait...");

    // Blocking HTTP must not run on the world thread, so do the round trip on
    // a scratch thread and report from there. Turns "the bots are quiet" into
    // a one-command diagnosis: you see the raw output and the cleaned output
    // side by side.
    std::thread([text]()
    {
        OllamaApiResult api = QueryOllama(text, OllamaRequestKind::ChatReply);

        if (!api.ok)
        {
            LOG_INFO("module.ollamachat", "[Ollama Chat] TEST FAILED after {}ms: {}",
                     api.latencyMs, api.error.empty() ? "unknown error" : api.error);
            return;
        }

        uint32_t emote = 0;
        const std::string cleaned = ProcessLlmResponse(api.text, "Tester", &emote);

        LOG_INFO("module.ollamachat", "[Ollama Chat] TEST ok in {}ms (think={}).",
                 api.latencyMs, api.thinkUsed ? "yes" : "no");
        LOG_INFO("module.ollamachat", "[Ollama Chat] TEST raw     : {}", api.text);
        LOG_INFO("module.ollamachat", "[Ollama Chat] TEST cleaned : {}", cleaned);
        if (emote)
            LOG_INFO("module.ollamachat", "[Ollama Chat] TEST emote   : {}", emote);
        if (!api.thinking.empty())
            LOG_INFO("module.ollamachat", "[Ollama Chat] TEST thinking: {}", api.thinking);
    }).detach();

    handler->PSendSysMessage("[Ollama Chat] Result will appear in the server log (module.ollamachat).");
    return true;
}
