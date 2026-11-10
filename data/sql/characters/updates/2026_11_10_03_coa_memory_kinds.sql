-- Conquest of Azeroth: kinds of memories, after "Generative Agents" (Park et al., 2023).
--   0  an experience or a note from a conversation
--   1  a reflection: what the bot concluded from what it went through ("Chris never leaves me behind in a fight.")
--   2  a goal: what the bot wants in life ("You want to save enough for a better sword.")
--   3  a rumour: what another bot told it about someone
ALTER TABLE mod_ollama_chat_memories
    ADD COLUMN kind TINYINT UNSIGNED NOT NULL DEFAULT 0 AFTER about_guid;
