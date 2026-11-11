-- Roleplay sheets (AFKRoleplay add-on): written by players for themselves, by the language model for bots.
CREATE TABLE IF NOT EXISTS mod_ollama_chat_rp (
    guid        BIGINT UNSIGNED NOT NULL,
    is_bot      TINYINT UNSIGNED NOT NULL DEFAULT 0,
    rp_name     VARCHAR(64)  NOT NULL DEFAULT '',
    title       VARCHAR(160) NOT NULL DEFAULT '',
    age         VARCHAR(80)  NOT NULL DEFAULT '',
    home        VARCHAR(160) NOT NULL DEFAULT '',
    appearance  TEXT NOT NULL,
    personality TEXT NOT NULL,
    history     TEXT NOT NULL,
    currently   TEXT NOT NULL,
    updated     INT UNSIGNED NOT NULL DEFAULT 0,
    PRIMARY KEY (guid)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- Two-word names ("Kegarink Stonehammer") are up to 25 characters.
ALTER TABLE mod_ollama_chat_identity MODIFY name VARCHAR(25) NOT NULL DEFAULT '';
