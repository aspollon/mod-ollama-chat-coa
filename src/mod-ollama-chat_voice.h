#ifndef MOD_OLLAMA_CHAT_VOICE_H
#define MOD_OLLAMA_CHAT_VOICE_H

#include <cstdint>
#include <string>

class Player;

// Companion Voice (client add-on): hidden "CVOX" labels so the player's client speaks the bot's line aloud.
// World thread only.
enum OllamaVoiceScope { VOICE_PARTY, VOICE_WHISPER, VOICE_SAY, VOICE_YELL };
void Voice_Label(Player* bot, Player* whisperTo, OllamaVoiceScope scope, std::string const& text);
void Voice_Update(uint32_t diff);

#endif
