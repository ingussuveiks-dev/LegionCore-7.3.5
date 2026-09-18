-- Both quest IDs are variants of The Battle for Shattrath and both lines use
-- Archmage Khadgar's creature_text. The 33731 row was assigned source type 1
-- (gameobject) even though 75805 is the creature template.
CREATE TABLE IF NOT EXISTS `_backup_20260918_khadgar_shattrath_source` LIKE `smart_scripts`;

INSERT IGNORE INTO `_backup_20260918_khadgar_shattrath_source`
SELECT *
FROM `smart_scripts`
WHERE `entryorguid` = 75805 AND `source_type` = 1 AND `id` = 0
  AND `event_type` = 19 AND `event_param1` = 33731 AND `action_type` = 1;

UPDATE `smart_scripts`
SET `source_type` = 0,
    `id` = 1,
    `comment` = 'Archmage Khadgar - On Quest 33731 Accepted - Say Line'
WHERE `entryorguid` = 75805 AND `source_type` = 1 AND `id` = 0
  AND `event_type` = 19 AND `event_param1` = 33731 AND `action_type` = 1;
