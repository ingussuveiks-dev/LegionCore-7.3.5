-- Shadowy Figure 119419 performs the Deadmines pet-battle roleplay through
-- timed action list 11941900. Its movement action references path 119419,
-- which is absent from all checked Legion databases and forks. Preserve the
-- unsupported movement row before disabling only it; all ten dialogue beats
-- and the final despawn remain active.
CREATE TABLE IF NOT EXISTS `_backup_20260919_incomplete_deadmines_shadowy_path` LIKE `smart_scripts`;

INSERT IGNORE INTO `_backup_20260919_incomplete_deadmines_shadowy_path`
SELECT *
FROM `smart_scripts`
WHERE `entryorguid` = 11941900
  AND `source_type` = 9
  AND `id` = 1
  AND `action_type` = 53
  AND `action_param2` = 119419;

DELETE FROM `smart_scripts`
WHERE `entryorguid` = 11941900
  AND `source_type` = 9
  AND `id` = 1
  AND `action_type` = 53
  AND `action_param2` = 119419;
