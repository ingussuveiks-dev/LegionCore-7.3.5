-- Durable action checkpoints and finale proof; no substitute quest objectives.
CREATE TABLE IF NOT EXISTS character_stormheim_mission (
 guid bigint unsigned NOT NULL,
 quest int unsigned NOT NULL,
 serial bigint unsigned NOT NULL,
 stage int unsigned NOT NULL DEFAULT 0,
 air int unsigned NOT NULL DEFAULT 0,
 captains int unsigned NOT NULL DEFAULT 0,
 flares int unsigned NOT NULL DEFAULT 0,
 frost int unsigned NOT NULL DEFAULT 0,
 final tinyint unsigned NOT NULL DEFAULT 0,
 PRIMARY KEY (guid,quest)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;
