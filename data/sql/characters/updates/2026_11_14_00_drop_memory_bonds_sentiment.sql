-- The module no longer keeps long-term memories, relationships, bonds or sentiment.
-- The code that read and wrote these tables is gone; drop what earlier versions stored.
DROP TABLE IF EXISTS mod_ollama_chat_memories;
DROP TABLE IF EXISTS mod_ollama_chat_relationships;
DROP TABLE IF EXISTS mod_ollama_chat_bonds;
DROP TABLE IF EXISTS mod_ollama_chat_bot_player_sentiments;
