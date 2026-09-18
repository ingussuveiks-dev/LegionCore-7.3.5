-- Seirdr's first combat spell had the repeat interval bounds reversed.
-- Keep both authored values and normalize them to a valid 2-6 second window.
CREATE TABLE IF NOT EXISTS `_backup_20260918_seirdr_combat_timer` LIKE `smart_scripts`;

INSERT IGNORE INTO `_backup_20260918_seirdr_combat_timer`
SELECT *
FROM `smart_scripts`
WHERE `entryorguid` = 115751
  AND `source_type` = 0
  AND `id` = 0
  AND `link` = 0;

UPDATE `smart_scripts`
SET `event_param3` = 2000,
    `event_param4` = 6000,
    `comment` = 'Seirdr - In Combat - Cast 186327 (2-6 Second Repeat)'
WHERE `entryorguid` = 115751
  AND `source_type` = 0
  AND `id` = 0
  AND `link` = 0
  AND `event_type` = 0
  AND `event_param1` = 1000
  AND `event_param2` = 1000
  AND `event_param3` = 6000
  AND `event_param4` = 2000
  AND `action_type` = 11
  AND `action_param1` = 186327
  AND `target_type` = 5;
