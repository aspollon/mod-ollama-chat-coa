-- Conquest of Azeroth: who a character guid belonged to when the module stored something about it.
--
-- Bots are deleted and created anew (a bot reset deletes them straight from the database), and the server can give
-- a new character the guid of a deleted one. Without this, a new bot would inherit the memories, temperament and
-- bonds of the bot that had its guid before, and a new player character the friendships of a deleted one. At every
-- start the module removes what belongs to characters that are gone or are no longer the same character.

CREATE TABLE IF NOT EXISTS mod_ollama_chat_identity (
    guid   BIGINT UNSIGNED NOT NULL,
    name   VARCHAR(12) NOT NULL DEFAULT '',
    race   TINYINT UNSIGNED NOT NULL DEFAULT 0,
    class  TINYINT UNSIGNED NOT NULL DEFAULT 0,
    gender TINYINT UNSIGNED NOT NULL DEFAULT 0,
    PRIMARY KEY (guid)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
