-- The linked cast row had the spell ID in target_param1, leaving action_param1
-- empty and treating spell 173313 as a creature entry. Preserve it before
-- restoring Reckoning against the same Balnazzar target as the linked attack.
CREATE TABLE IF NOT EXISTS `_backup_20260918_lothraxion_reckoning_target` LIKE `smart_scripts`;

INSERT IGNORE INTO `_backup_20260918_lothraxion_reckoning_target`
SELECT *
FROM `smart_scripts`
WHERE `entryorguid` = 111343 AND `source_type` = 0 AND `id` = 2
  AND `event_type` = 61 AND `action_type` = 11 AND `action_param1` = 0
  AND `target_type` = 19 AND `target_param1` = 173313;

UPDATE `smart_scripts`
SET `action_param1` = 173313,
    `target_param1` = 111247,
    `comment` = 'Link - Cast Reckoning on Balnazzar'
WHERE `entryorguid` = 111343 AND `source_type` = 0 AND `id` = 2
  AND `event_type` = 61 AND `action_type` = 11 AND `action_param1` = 0
  AND `target_type` = 19 AND `target_param1` = 173313;
