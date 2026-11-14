<p align="center">
  <img src="./icon.png" alt="Ollama Chat Module" title="Ollama Chat Module Icon">
</p>


# mod-ollama-chat-coa

**mod-ollama-chat for Conquest of Azeroth and AFK Realm.** This is [mod-ollama-chat](https://github.com/DustinHendrickson/mod-ollama-chat) by Dustin Hendrickson, made to build and run on the [Conquest of Azeroth](https://github.com/Conquest-of-AzerothCore/azerothcore-wotlk-coa) core with [Zyth45's Playerbots](https://github.com/Zyth45/mod-playerbots/tree/coa), and installed and set up by [AFK Realm](https://github.com/aspollon/AFK-Realm). Like the original, it is free software under the GNU Affero General Public License v3 (see `LICENSE`); the original's copyright stays with its author.

What is different from the original:

- It builds on the Conquest of Azeroth core (whose sessions name a headless bot `IsHeadlessBot()`).
- The module is called `mod-ollama-chat-coa`: put it into `modules/mod-ollama-chat-coa` (its loader is `Addmod_ollama_chat_coaScripts`). Its configuration file and database tables keep the original's names (`mod_ollama_chat.conf`, `mod_ollama_chat_*`).
- `afk-realm.json`: AFK Realm turns off the prewritten chatter that would talk over the module: the bots' own lines of mod-playerbots, the chatter of mod-playerbots-auctions, and the lines of the Immersion residents (legendaries, beggars, buyers, anglers) and of its PvP bots. The Trade channel is left to trade.
- Every CoA and CoA Custom race and every CoA class has a voice (how it speaks, what it cares about); race and class names come from the server's own tables. Every bot gets a temperament of its own.
- Conversations: a person who targets a bot, or who is talking with one, is answered by that bot alone, without naming it every time. Names count the way people type them (`klaus` or `klaus dieter` for Klausdieter, one typo). A line said to nobody in particular gets one answer, not a chorus. The conversation so far (the last ten exchanges) and the new line come last in the prompt, so the bot keeps the thread.
- Bots know what they are doing: the time of day, a fight, a flight, riding, resting, the dungeon they are in, their companions and open quests, and how close the person is.
- Bots speak to people first: when a bot's turn to talk comes while a person stands near it, it may speak to them by name (`OllamaChat.Approach.Chance`, `OllamaChat.Approach.CooldownMinutes`); the person answers without naming it.
- Spoken words and actions are separated by double quotes: everything the bot says is inside the quotes, everything outside them is an emote line in the third person ("Klausdieter grins wickedly.") sent before the spoken words. Asterisks and bare stage directions are still recognised for models that do not quote.
- Answers that repeat the prompt or the bot's last line are asked for again.
- Bots know the world, built into the module (no files, no RAG): the story of every CoA and CoA Custom people, the order or tradition of every CoA class, every zone of the old world, Outland and Northrend with its capitals, and the persons, orders and events of the time of the Lich King. A bot knows its own people, calling and place, something of the people of whoever it talks to, and what a message names ("Arthas", "Gilneas", "the Scourge"). With mod-world-journey it knows how far along the road to Outland and Northrend it is, in words, never in levels (`OllamaChat.Lore.Enable`, `OllamaChat.Lore.MaxTopics`). The tables are in `src/mod-ollama-chat_lore_data.cpp`.
- `OllamaChat.NumCtx = 8192`: room for all of that; with a model's short default the instructions at the start of the prompt were cut off.
- A bot in a conversation stays: it stops its errands, turns to the person and follows a few steps when they walk on, until the conversation goes quiet or the person says goodbye and the bot has answered; never in combat, dungeons, raids, battlegrounds or duels (`OllamaChat.Conversation.HoldSeconds`). Immersion's residents keep their place instead.
- `src/CoaChatBridge.h` connects the module with mod-playerbots-auctions and mod-playerbots-immersion without either needing the other: the residents of Immersion tell the bot who it is (a beggar, a legendary veteran, an angler, someone at the bank) and hand over their ambient lines; Auctions hands over the whispers of a deal (items, amounts and prices stay exactly as they are, or the original line is sent) and tells how the bot trades (`OllamaChat.Voice.Enable`).
- Deleted characters take what the module stored about them along - their own history and temperament, and their roleplay sheet - when they are deleted, or at the next start when a bot reset deleted them straight from the database. A guid the server gives to a new character starts with a clean slate.
- The original's RAG files (`data/rag`, World of Warcraft as players talk about it) stay switched off.

The rest of this file is the original's documentation; where it says `mod-ollama-chat`, the folder is `mod-ollama-chat-coa` here.

---

# AzerothCore + Playerbots Module: mod-ollama-chat


> [!CAUTION]
> **LLM/AI Disclaimer:** Large Language Models (LLMs) such as those used by this module do not possess intelligence, reasoning, or true understanding. They generate text by predicting the most likely next word based on patterns in their training data—matching vectors, not thinking or comprehension. The quality and relevance of responses depend entirely on the model you use, its training data, and its configuration. Results may vary, and sometimes the output may be irrelevant, nonsensical, or simply not work as expected. This is a fundamental limitation of current AI and LLM technology. Use with realistic expectations.
>
> This module is also in development and can bog down your server due to the nature of running local LLM. Please proceed with this in mind.

> [!IMPORTANT]
> To fully disable Playerbots normal chatter and random chatter that might interfere with this module, set the following settings in your `playerbots.conf`:
> - `AiPlayerbot.EnableBroadcasts = 0` (disables loot/quest/kill broadcasts)
> - `AiPlayerbot.RandomBotTalk = 0` (disables random talking in say/yell/general channels)
> - `AiPlayerbot.RandomBotEmote = 0` (disables random emoting)
> - `AiPlayerbot.RandomBotSuggestDungeons = 0` (disables dungeon suggestions)
> - `AiPlayerbot.EnableGreet = 0` (disables greeting when invited)
> - `AiPlayerbot.GuildFeedback = 0` (disables guild event chatting)
> - `AiPlayerbot.RandomBotSayWithoutMaster = 0` (disables bots talking without a master)
> - `AiPlayerbot.CoaLive.Chat.Enable = 0` (CoA playerbots: disables CoA Live, its own prewritten chat; this module's guild lines still go out)

## Overview

***mod-ollama-chat*** is an AzerothCore module that enhances the Player Bots module by integrating external language model (LLM) support via the Ollama API. This module enables player bots to generate dynamic, in-character chat responses using advanced natural language processing locally on your computer (or remotely hosted). Bots are enriched with personality traits, random chatter triggers, and context-aware replies that mimic the language and lore of World of Warcraft.

## Features

- **Ollama LLM Integration:**  
  Bots generate chat responses by querying an external Ollama API endpoint. This enables natural and contextually appropriate in-game dialogue.

- **Other LLM Providers:**  
  Not limited to Ollama. Set `OllamaChat.Provider` to `openai` for anything that speaks the OpenAI Chat Completions format (OpenAI, OpenRouter, Groq, Mistral, DeepSeek, xAI, Gemini, LM Studio, vLLM, llama.cpp) or to `anthropic` for Claude. See [Using Other LLM Providers](#using-other-llm-providers-openai-openrouter-claude).

- **Player Bot Personalities:**  
  When enabled, each bot is assigned a personality type (e.g., Gamer, Roleplayer, Trickster) that modifies its chat style. Personalities influence prompt generation and result in varied, immersive responses.

- **Context-Aware Prompt Generation:**  
  The module gathers extensive context about both the bot and the interacting player—including class, race, role, faction, guild, and more—to generate prompts for the LLM. A comprehensive WoW cheat sheet is appended to every prompt to ensure the LLM replies with accurate lore, terminology, and in-character language spanning Vanilla WoW, The Burning Crusade, and Wrath of the Lich King.

- **Random Chatter:**  
  Bots can periodically initiate random, environment-based chat when a real player is nearby. This feature adds an extra layer of immersion to the game world.

- **Chat Memory (Conversation History):**  
  Bots now have configurable short-term chat memory. Recent conversations between each player and bot are stored and included as context in every LLM prompt, giving responses better context and continuity.

  Bots now recall your recent interactions—responses will reflect the last several lines of chat with each player.

- **Blacklist for Playerbot Commands:**  
  A configurable blacklist prevents bots from responding to chat messages that start with common playerbot command prefixes, ensuring that administrative commands are not inadvertently processed. Additional commands can be appended via the configuration.

- **Asynchronous Response Handling:**  
  Chat responses are generated on separate threads to avoid blocking the main server loop, ensuring smooth server performance.

- **Live Configuration & Personality Reload:**  
  Reload the module’s config and personality packs in-game or from the server console, without restarting.

- **Event-Based Chatter:**  
  Player bots now comment on key in-game events such as quest completion, rare loot, deaths, PvP kills, leveling up, duels, learning spells, and achievements. Remarks are context-aware, immersive, and personality-driven, making the world feel much more alive.

- **Party-Only Bot Responses:**  
  When enabled, bots will only respond to real player messages and events when they are in the same non-raid party. This helps reduce chat spam while maintaining full bot-to-bot communication within parties for immersive group interactions.

- **Think Mode Support:**  
  Bots can leverage LLM models that have reasoning/think modes. Enable internal reasoning for models that support it by setting `OllamaChat.ThinkModeEnableForModule = 1` in **mod-ollama-chat.conf**. When enabled, the API request includes the `think` flag and the bot omits all `thinking` responses from its final reply.

- **Live Reload for Personalities and Settings:**  
  Instantly reload all mod-ollama-chat configuration and personality packs in-game using the `.ollama reload` command with a GM level account or use `ollama reload` from the server console. No server restart required—updates to `.conf` or personality packs (`.sql` files) are applied immediately.

## Installation

> [!IMPORTANT]
> **Cross-Platform Support**: This module now uses cpp-httplib (header-only) instead of curl, eliminating compilation issues on Windows and simplifying installation on all platforms.

1. **Prerequisites:**
   - Ensure you have liyunfan1223's AzerothCore (https://github.com/liyunfan1223/azerothcore-wotlk) installation with the Player Bots (https://github.com/liyunfan1223/mod-playerbots) module enabled.
   - The module depends on:
     - **fmtlib** (https://github.com/fmtlib/fmt) - For string formatting
     - **nlohmann/json** (https://github.com/nlohmann/json) - For JSON processing (**bundled with module** - no installation needed)
     - cpp-httplib (https://github.com/yhirose/cpp-httplib) - Header-only HTTP library (included, no installation needed)
     - Ollama LLM support – set up a local instance of the Ollama API server with the model of your choice. More details at https://ollama.com

2. **Install Dependencies:**

   ### Windows (vcpkg):
   ```bash
   vcpkg install fmt
   ```

   ### Ubuntu/Debian:
   ```bash
   sudo apt update
   sudo apt install libfmt-dev
   ```

   ### CentOS/RHEL/Fedora:
   ```bash
   sudo yum install fmt-devel  # or dnf install fmt-devel
   ```

   ### macOS (Homebrew):
   ```bash
   brew install fmt
   ```

   ### Arch Linux:
   ```bash
   sudo pacman -S fmt
   ```

3. **Clone the Module:**
   ```bash
   cd /path/to/azerothcore/modules
   git clone https://github.com/DustinHendrickson/mod-ollama-chat.git
   ```

4. **Recompile AzerothCore:**
   ```bash
   cd /path/to/azerothcore
   mkdir build && cd build
   cmake ..
   make -j$(nproc)
   ```

5. **Configuration:**
   Copy the default configuration file to your server configuration directory and change to match your setup (if not already done):
   ```bash
   cp /path/to/azerothcore/modules/mod-ollama-chat/conf/mod_ollama_chat.conf.dist /path/to/azerothcore/env/dist/etc/modules/mod_ollama_chat.conf
   ```

6. **Restart the Server:**
   ```bash
   ./worldserver
   ```

## Setting up Ollama Server

This module requires a running Ollama server to function. Ollama allows you to run large language models locally on your machine.

### Installing Ollama

Download and install Ollama from [ollama.com](https://ollama.com). It supports Windows, macOS, and Linux.

- **Windows/macOS:** Download the installer from the website and run it.
- **Linux:** Follow the installation instructions for your distribution (e.g., `curl -fsSL https://ollama.com/install.sh | sh`).

### Starting the Ollama Server

Once installed, start the Ollama server:

```bash
ollama serve
```

This will start the server on `http://localhost:11434` by default.

### Running Ollama Across the Network

If you want to run the Ollama server on a different computer than your AzerothCore server, set the `OLLAMA_HOST` environment variable to `0.0.0.0` before starting the server:

```bash
export OLLAMA_HOST=0.0.0.0
ollama serve
```

This binds the server to all network interfaces, allowing connections from other machines on your network. Update the `OllamaChat.Url` in `mod-ollama-chat.conf` to use the IP address of the machine running Ollama (e.g., `http://192.168.1.100:11434`).

> [!WARNING]
> Exposing Ollama to the network may pose security risks. Ensure your firewall allows traffic on port 11434 only from trusted networks, and consider additional security measures if exposing to the internet.

### Pulling a Model

Before using the module, pull a model that the bots will use for generating responses. For example, to pull the Llama 3.2 1B model:

```bash
ollama pull llama3.2:1b
```

You can find available models at [ollama.com/library](https://ollama.com/library). Choose a model that fits your hardware capabilities.

### Connecting the Module

The module connects to the Ollama API via the configuration in `mod-ollama-chat.conf`. The default endpoint is `http://localhost:11434`. If your Ollama server is running on a different host or port, update the `OllamaChat.Url` setting.

### Checking if Ollama is Running

To verify that the Ollama server is running and accessible, you can test the API:

```bash
curl http://localhost:11434/api/tags
```

This should return a JSON response listing available models. If you get a connection error, ensure the server is started and the endpoint is correct.

## Using Other LLM Providers (OpenAI, OpenRouter, Claude)

Ollama is the default, but it is not required. The only Ollama-specific part
of the module is how a single request is encoded, so any service that speaks
one of two common API formats can be used instead. Four settings in
`mod-ollama-chat.conf` control this:

| Setting | Meaning |
|---|---|
| `OllamaChat.Provider` | `ollama` (default), `openai`, or `anthropic` |
| `OllamaChat.Url` | The full URL of the provider's generation endpoint |
| `OllamaChat.Model` | The model name as the provider publishes it |
| `OllamaChat.ApiKey` | The provider's API key. Leave empty for local servers that need none |
| `OllamaChat.ApiKeyHeader` | Optional. Only for services with their own header name, such as Azure OpenAI (`api-key`) |

`openai` means the **OpenAI Chat Completions format**, which is the de facto
standard and is spoken by far more than OpenAI: OpenRouter, Groq, Together,
Mistral, DeepSeek, xAI, Google Gemini (through its OpenAI-compatible
endpoint), LM Studio, vLLM, llama.cpp server, and Ollama's own
`/v1/chat/completions` route all accept it. `anthropic` is the native Claude
Messages API. Claude is also reachable through OpenRouter using `openai`.

### Examples

**OpenAI**

```ini
OllamaChat.Provider = openai
OllamaChat.Url      = https://api.openai.com/v1/chat/completions
OllamaChat.Model    = gpt-4o-mini
OllamaChat.ApiKey   = sk-...
```

**OpenRouter** (one key, access to many models from many vendors)

```ini
OllamaChat.Provider = openai
OllamaChat.Url      = https://openrouter.ai/api/v1/chat/completions
OllamaChat.Model    = openai/gpt-4o-mini
OllamaChat.ApiKey   = sk-or-...
```

**Anthropic (Claude)**

```ini
OllamaChat.Provider = anthropic
OllamaChat.Url      = https://api.anthropic.com/v1/messages
OllamaChat.Model    = claude-haiku-4-5-20251001
OllamaChat.ApiKey   = sk-ant-...
```

**LM Studio** (local, no key needed)

```ini
OllamaChat.Provider = openai
OllamaChat.Url      = http://localhost:1234/v1/chat/completions
OllamaChat.Model    = <the model id LM Studio shows in its server tab>
```

### Steps

1. Edit the four settings above in `mod-ollama-chat.conf`.
2. Restart `worldserver`, or run `.ollama reload` in game or `ollama reload`
   on the console.
3. Run `.ollama status`. The first line now shows the provider and whether a
   key is set (the key itself is never printed).
4. Run `.ollama test hello` to send one request and see the reply, or the
   provider's error message if something is wrong (bad key, wrong model name,
   wrong URL).

### What to know

- **HTTPS requires an OpenSSL build.** The CMake output says
  `[mod-ollama-chat] OpenSSL found - HTTPS support enabled` when it is, and
  `.ollama status` shows `HTTPS: available`. Without it, an `https://` URL is
  reported as an error at startup and every request fails. On Windows install
  OpenSSL (for example `vcpkg install openssl` or the Win64 OpenSSL installer)
  and re-run CMake; on Linux install `libssl-dev` or `openssl-devel`.
- **Certificates are verified when a key is set.** By default the module skips
  TLS certificate checks (so self-signed reverse proxies in front of a local
  Ollama keep working) but turns them on whenever `OllamaChat.ApiKey` is
  non-empty, since a key should never travel over an unverified connection.
  `OllamaChat.VerifyCertificates` overrides this either way.
- **API keys.** Every hosted provider needs one; put it in `OllamaChat.ApiKey`.
  The module sends it as a bearer token for OpenAI format and as
  `x-api-key` for Anthropic; `OllamaChat.ApiKeyHeader` overrides the header
  name for services like Azure OpenAI. A rejected key (HTTP 401/403) is
  reported once in the log with what to check.
- **Cost and rate limits.** Random chatter and event chatter across many bots
  adds up fast on a metered API, and hosted providers answer bursts with
  HTTP 429. The module waits and retries a bounded number of times
  (`OllamaChat.RateLimitRetries`, honouring the provider's `Retry-After`,
  capped by `OllamaChat.RateLimitMaxWaitSeconds`) and logs the first
  occurrence. Start with `OllamaChat.EnableRandomChatter = 0` and a low
  `OllamaChat.MaxConcurrentQueries`, and watch the provider's usage page before
  opening it up.
- **Settings that do not apply are not sent.** `NumCtx`, `NumThreads`,
  `RepeatPenalty`, `MinP` and `TopK` have no equivalent in the OpenAI format;
  Anthropic additionally has no `TopP`, `Seed`, or penalties. `NumPredict`
  works everywhere (Anthropic requires a cap, so `0` becomes 1024 there).
- **Think mode.** There is no capability probe for these providers, since
  that would be a billed request. Under `ThinkMode = auto` reasoning is never
  requested. Under `on` it is requested on every call (`reasoning_effort` for
  OpenAI format, an extended-thinking budget for Anthropic) and switched off
  for the session if the provider rejects it.
- **Self-healing parameters.** Newer OpenAI models reject `max_tokens` in
  favour of `max_completion_tokens`, and OpenAI reasoning models reject
  explicit sampling values. Both are detected from the first rejection, logged
  once, and the request is retried immediately, so no reply is lost.
- **Keep the key private.** Treat `mod-ollama-chat.conf` like
  `worldserver.conf`: readable only by the account running the server, and
  never committed to a repository.

## Configuration Options

> For a complete list of all available configuration options with comments and defaults, see `mod-ollama-chat.conf.dist` included in this repository.

## Text Commands

The module provides several in-game text commands for administrators (Game Masters) to manage and monitor the Ollama chat functionality. All commands require **SEC_ADMINISTRATOR** security level (GM level 3 or higher).

### `.ollama reload`
Reloads the module's configuration from `mod-ollama-chat.conf` without restarting the server. Also reloads personality packs.
- **Security Level:** SEC_ADMINISTRATOR
- **Usage:** `.ollama reload`
- **Console Equivalent:** `ollama reload`

### `.ollama personality get <bot_name>`
Displays the current personality assigned to a bot.
- **Security Level:** SEC_ADMINISTRATOR
- **Usage:** `.ollama personality get BotName`
- **Console Equivalent:** `ollama personality get <bot>`

### `.ollama personality set <bot_name> <personality>`
Manually assigns a personality to a bot.
- **Security Level:** SEC_ADMINISTRATOR
- **Usage:** `.ollama personality set BotName Gamer`
- **Console Equivalent:** `ollama personality set <bot> <personality>`

### `.ollama personality list`
Lists all available personalities and their descriptions.
- **Security Level:** SEC_ADMINISTRATOR
- **Usage:** `.ollama personality list`
- **Console Equivalent:** `ollama personality list`

> [!NOTE]
> All commands can also be executed from the server console by replacing the leading dot (.) with the command prefix used in your console (typically none or a custom prefix).

### `.ollama status`

Shows what the module currently thinks it is doing. Reports the endpoint and
model, whether think mode is supported and why, dispatcher queue depth and
worker count, delivery and drop counters, governor state (including *why*
replies were suppressed), and the last error.

This is the first thing to run when bots go quiet.

### `.ollama test <prompt>`

Sends one prompt straight to Ollama and writes the raw output and the
post-processed output side by side to the server log (`module.ollamachat`),
along with the round-trip time and whether think mode was used. Turns "the bots
aren't talking" into a one-command diagnosis.

## How It Works

1. **Chat Filtering and Triggering**  
   When a player (or bot) sends a chat message, the module checks the message's type, distance, and if it starts with any configured blacklist command prefix. If party restrictions are enabled, only bots in the same non-raid party as the real player can respond. Only eligible messages in range and not matching the blacklist will trigger a bot response.

2. **Bot Selection**  
   The system gathers all bots within the relevant distance, determines eligibility based on player/bot reply chance, and caps responses per message using `MaxBotsToPick` and related settings.

3. **Prompt Assembly**  
   For each reply, a prompt is assembled by combining configurable templates with live in-game context: bot/player class, race, gender, role/spec, faction, guild, level, zone, gold, group, environment info, personality, and if enabled, recent chat history between that player and the bot.

4. **LLM Request**  
   The prompt is sent to the Ollama API using the configured model and parameters. All LLM requests run asynchronously, ensuring no lag or blocking of the server.

5. **Response Routing**  
   Bot responses are routed back through the appropriate chat channel in game, whether it’s say, yell, party or general.

6. **Personality Management**  
   If RP personalities are enabled, each bot uses its assigned personality template. Personality definitions can be changed on the fly and reloaded live—no server restart required.

7. **Random & Event-Based Chatter**  
   In addition to responding to direct chat, bots will occasionally generate random environment-aware lines when real players are nearby, and will also react to key in-game events (e.g., PvP/PvE kills, loot, deaths, quests, duels, level-ups, achievements, using objects) using context-specific templates and personalities.

8. **Live Reloading**  
   You can hot-reload the module config and personality packs in-game using the `.ollama reload` GM command or from the server console. All changes take effect immediately without requiring a restart.

9. **Fully Configurable**  
   All settings—reply logic, distances, frequencies, blacklist, prompt templates, chat history, personalities, random/event chatter, LLM params, and more—are controlled via `mod-ollama-chat.conf` and can be adjusted and reloaded live at any time.

## Personality Packs

`mod-ollama-chat` supports Personality Packs, which are collections of personality templates that define how bots roleplay and interact in-game.

- To use a Personality Pack, download or create a `.sql` file named in the format `YYYY_MM_DD_personality_pack_NAME.sql`.

- Place the `.sql` file in `modules/mod-ollama-chat/data/sql/characters/updates/`.

- The module will automatically detect and apply any new Personality Packs when the server starts or updates—no manual SQL import required.

Want to create your own pack or download packs made by the community?  

Visit the [Personality Packs Discussion Board](https://github.com/DustinHendrickson/mod-ollama-chat/discussions)

## Debugging

For detailed logs of bot responses, prompt generation, and LLM interactions, enable debug mode via your server logs or module-specific settings.



## Conversation Control

Bot chat can run away in several different ways, so there are four independent
brakes. All are configurable; see the `CONVERSATION GOVERNOR` section of the
config file.

| Brake | What it stops |
|---|---|
| **Chain depth + decay** | A bot replying to a bot replying to a bot. Each hop also multiplies the reply chance down, so chains lose energy before hitting the hard ceiling. |
| **The audience rule** | Bots holding conversations with nobody listening. Bots may only reply to *other bots* while a real player has spoken in that channel recently. This does most of the work. |
| **Cooldowns and rate limits** | One bot, or one crowd, dominating a channel. Per-bot, per-channel, and server-wide. The global limit also caps your LLM spend. |
| **Repetition scoring** | The same line twice, and the same *opening phrase* twice. Candidate replies are scored against the bot's own recent lines and the channel's recent traffic. |

If bots are looping, the setting to reach for first is
`OllamaChat.BotConversation.RequireRecentHuman`.

## What Bots Talk About

Topics are chosen from weighted categories rather than uniformly, and each bot
suppresses whatever it used in its last few picks.

| Category | Default weight | Examples |
|---|---|---|
| **People** | 30 | Nearby players by name, class and what they are doing; group members; guildmates online; something the bot just watched happen |
| **World** | 30 | The nearest *interesting* creature (elite, rare, or a real level threat — never a critter); named NPCs by role; landmarks; corpses; time of day |
| **Activity** | 25 | Current quest objectives; danger assessment; group needs (someone low, someone out of mana) |
| **Self** | 15 | Spells, equipped items, bag space — the original topics, kept but demoted |

**Witnessed-event memory.** Bots keep a short memory of what they saw happen
near them — kills, deaths, level-ups, loot. This is what lets a bot comment on
the fight you were both just in rather than reciting a fact about itself.

Note that `OllamaChat.Snapshot.IncludeSpells` now defaults to **0**. Listing
every off-cooldown spell a bot knew put dozens of lines of the most quotable
text in the prompt, which is why bots talked about their spellbook so much.

## Roleplay Mode

Off by default. Turn on with `OllamaChat.Roleplay.Enable`.

Gives each race a speech register and cultural touchstones, and each class a
worldview — what that character *notices*. A Tauren speaks slowly of the
Earthmother and the balance; a Forsaken is dry and calls the living "breathers";
a priest sees wounds, a hunter reads tracks, a rogue counts exits.

`OllamaChat.Roleplay.Strictness` controls how far it goes:

- **0** — Flavour only. Voices colour the prompt, nothing else changes.
- **1** — In character. Out-of-world vocabulary (`dps`, `nerf`, `patch`, …) is
  rejected rather than spoken, and faction attitude is applied.
- **2** — Hard in character. In-world chatter lists replace the shipped
  out-of-character ones, injuries and distances are described rather than
  quoted as figures, and think mode is used where the model supports it.

`OllamaChat.Roleplay.CrossFactionGibberish` (on by default in roleplay mode)
stops bots answering across factions in say/yell — the client renders those as
gibberish anyway, so a fluent reply is the most immersion-breaking thing the
module can do.

Race and class voices can be overridden per server in the
`mod_ollama_chat_voice` table without a rebuild.

## Body Language

Replies are accompanied by movement so they read as conversation rather than as
a log line.

- **Facing** — the bot turns toward whoever it is answering, just before the
  line lands. Skipped while moving, casting, in combat, in flight, on a
  transport, or teleporting.
- **Actions and gestures** — what a bot does is a text emote (`/em ...`), never
  part of the spoken words. The model writes everything it says inside double
  quotes and everything it does outside them, in the third person:
  `*rutscht auf dem Stuhl hin und her und kichert* "Hallo." The module sends the
  outside text as an emote and says the quoted words as the chat line. Actions
  the model slips into the middle of a sentence are removed from the speech and
  sent as the emote as well. `[emote:wave]`, `*waves*` and a bare `/wave` are
  still recognised as optional game animations, because models emit those
  unprompted; asterisks and bare stage directions remain a fallback for models
  that do not quote. Emotes are played through the same code path the client
  uses, so nearby players see both the animation and the "Bot waves at You."
  social text, with the correct per-race and per-gender variant.
- **Emote reactions** — bots react when a player emotes at them, either by
  mirroring (wave gets a wave), countering (flex gets a laugh), or answering in
  words. The first two cost no LLM call.

## Think Mode

`OllamaChat.ThinkMode` takes `auto` (default), `on`, or `off`.

Under `auto` the module asks Ollama what the configured model can actually do
and only ever sends `think` to a model that reports the capability — so a model
that cannot think is never asked to. It then spends reasoning only where it
changes the answer: off for short chat lines, on for roleplay sheets and
strict roleplay replies.

If a live request is ever rejected for asking to think, the module remembers
that, logs it once, and retries without it. A latency guard
(`OllamaChat.ThinkMaxLatencyMs`) backs think mode off for the session if it
proves too slow for chat. You do not need to restart after swapping models —
`.ollama reload` re-probes.

With a non-Ollama provider (`OllamaChat.Provider = openai` or `anthropic`)
nothing is probed, because the only way to ask would be a billed request.
`auto` then never requests reasoning; `on` requests it on every call and backs
off for the session if the provider rejects it.

## Threading Model

Worker threads do HTTP and string work only. Every read or write of a `Player`,
`Channel`, `Guild`, `Group` or `Map` happens on the world thread.

Requests are built on the world thread, handed to a bounded worker pool
(`OllamaChat.WorkerThreads`), and delivered back on the world thread by a
completion queue drained each tick. Queue depth is capped
(`OllamaChat.MaxQueueDepth`) so a slow Ollama sheds load instead of building a
backlog of stale replies.

## License

This module is released under the GNU GPL v3 license, consistent with AzerothCore's licensing.

## Contribution

Developed by Dustin Hendrickson

Pull requests, bug reports, and feature suggestions are welcome. Please adhere to AzerothCore's coding standards and guidelines when submitting contributions.

## Roleplay sheets (AFKRoleplay)

The client add-on `client/AddOns/AFKRoleplay` (AFK Realm installs it with the module) shows a roleplay sheet for every character: `/rp` or the **RP** button on the target frame or the inspect window. Write your own with `/rp edit`; bots get theirs written by the language model (ahead of time while it is idle, else when opened). Bots read both when they talk. Settings: `OllamaChat.RP.*`. Details: `docs/2026-11-11-roleplay-sheets.md`.

## Bot surnames

`AiPlayerbot.CoaBotSurname` (playerbots.conf) chooses the random bots' surname: `1` the surname "Bot", `2` a family name, `0` none. mod-playerbots draws the family names of `2` from one neutral list; with AFK Realm the patch `patches/playerbots-bot-surnames.patch` gives a bot a surname of its people instead, put together from two parts (Stone+hammer, Moon+whisper: 200 to 570 per people) and drawn from its first name, so it never changes. A list of the realm's own in `AiPlayerbot.CoaBotSurnameList` still wins. Applied at the next start; sheets and personalities are keyed by the character and stay through a change of name.
