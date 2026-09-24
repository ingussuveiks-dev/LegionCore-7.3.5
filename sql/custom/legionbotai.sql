-- LegionBotAI for the local legion_* databases.
-- Run only after checking that GUIDs 900000-900003 and 900010-900013
-- and the listed character names are free. This file never deletes data.
USE `legion_auth`;
INSERT INTO `account` (`username`, `sha_pass_hash`, `sessionkey`, `v`, `s`, `email`, `expansion`)
SELECT 'LEGIONBOTAI', SHA2(UUID(), 256), '', '', '', 'bot@localhost', 6
WHERE NOT EXISTS (SELECT 1 FROM `account` WHERE `username` = 'LEGIONBOTAI');

SET @legionbot_account_id := (SELECT `id` FROM `legion_auth`.`account` WHERE `username` = 'LEGIONBOTAI');
USE `legion_characters`;

CREATE TABLE IF NOT EXISTS `character_legionbot_settings` (
    `guid` INT UNSIGNED NOT NULL,
    `level_mode` TINYINT UNSIGNED NOT NULL DEFAULT 0,
    `fixed_level` TINYINT UNSIGNED NOT NULL DEFAULT 1,
    `player_tank` TINYINT UNSIGNED NOT NULL DEFAULT 0,
    `assist_mode` TINYINT UNSIGNED NOT NULL DEFAULT 0,
    `hold_position` TINYINT UNSIGNED NOT NULL DEFAULT 0,
    PRIMARY KEY (`guid`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

INSERT INTO `characters`
    (`guid`, `account`, `name`, `race`, `class`, `gender`, `level`, `map`,
     `position_x`, `position_y`, `position_z`, `orientation`, `taximask`,
     `online`, `specialization`, `health`, `mana`)
SELECT bots.guid, @legionbot_account_id, bots.name, bots.race, bots.class, bots.gender, 1,
       bots.map, bots.x, bots.y, bots.z, 0, '', 0, bots.spec, 100, 100
FROM (
    SELECT 900000 guid, 'Bulwark' name, 9 race, 6 class, 0 gender, 1 map, 1629.36 x, -4373.63 y, 31.2 z, 250 spec
    UNION ALL SELECT 900001, 'Lovley', 10, 2, 1, 1, 1629.36, -4373.63, 31.2, 65
    UNION ALL SELECT 900002, 'Ember', 5, 1, 1, 1, 1629.36, -4373.63, 31.2, 72
    UNION ALL SELECT 900003, 'Faith', 5, 5, 1, 1, 1629.36, -4373.63, 31.2, 257
    UNION ALL SELECT 900010, 'Aegis', 1, 6, 0, 0, -8949.95, -132.493, 83.5312, 250
    UNION ALL SELECT 900011, 'Seraphine', 11, 2, 1, 0, -8949.95, -132.493, 83.5312, 65
    UNION ALL SELECT 900012, 'Rook', 1, 1, 0, 0, -8949.95, -132.493, 83.5312, 72
    UNION ALL SELECT 900013, 'Elowen', 4, 5, 1, 0, -8949.95, -132.493, 83.5312, 257
) AS bots
WHERE NOT EXISTS (SELECT 1 FROM `characters` c WHERE c.guid = bots.guid OR c.name = bots.name);

INSERT INTO `character_homebind` (`guid`, `mapId`, `zoneId`, `posX`, `posY`, `posZ`)
SELECT c.guid, IF(c.map = 1, 1, 0), IF(c.map = 1, 1637, 1519),
       c.position_x, c.position_y, c.position_z
FROM `characters` c
LEFT JOIN `character_homebind` h ON h.guid = c.guid
WHERE c.account = @legionbot_account_id AND h.guid IS NULL;
