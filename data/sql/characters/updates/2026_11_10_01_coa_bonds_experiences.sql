-- Conquest of Azeroth: what bots went through, and how close they are to the people they know.
--
-- mod_ollama_chat_memories.about_guid: an experience with this person (a fight, a quest, a trade), the bot's own
-- guid for its own doings, 0 for the notes condensed from conversations.
--
-- mod_ollama_chat_bonds: one row per bot and person. affinity -100 (enemy) to 100 (closest friend), romance 0 to
-- 100, how often they talked and went through something together, when they last spoke and what about.

ALTER TABLE mod_ollama_chat_memories
    ADD COLUMN about_guid BIGINT UNSIGNED NOT NULL DEFAULT 0 AFTER bot_guid,
    ADD INDEX idx_bot_about (bot_guid, about_guid);

CREATE TABLE IF NOT EXISTS mod_ollama_chat_bonds (
    bot_guid    BIGINT UNSIGNED NOT NULL,
    other_guid  BIGINT UNSIGNED NOT NULL,
    other_name  VARCHAR(64) NOT NULL DEFAULT '',
    affinity    SMALLINT NOT NULL DEFAULT 0,
    romance     SMALLINT NOT NULL DEFAULT 0,
    talks       INT UNSIGNED NOT NULL DEFAULT 0,
    shared      INT UNSIGNED NOT NULL DEFAULT 0,
    last_topic  VARCHAR(255) NOT NULL DEFAULT '',
    last_seen   BIGINT UNSIGNED NOT NULL DEFAULT 0,
    PRIMARY KEY (bot_guid, other_guid),
    INDEX idx_bot (bot_guid)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
