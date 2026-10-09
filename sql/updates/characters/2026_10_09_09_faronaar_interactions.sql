-- Distinct crystal/lock uses survive relog. Ordinals allow reconciliation if
-- quest progress was rolled back or the quest was abandoned and accepted again.
CREATE TABLE IF NOT EXISTS character_faronaar_interactions (
    guid INT UNSIGNED NOT NULL,
    quest INT UNSIGNED NOT NULL,
    spawn BIGINT UNSIGNED NOT NULL,
    ordinal TINYINT UNSIGNED NOT NULL,
    PRIMARY KEY (guid,quest,spawn)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;
