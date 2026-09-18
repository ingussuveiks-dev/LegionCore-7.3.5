-- A later SQL merge duplicated Kalecgos' SetData(0,2) handler with the same
-- id and payload but link=1. The original link=0 handler is already active;
-- the duplicate can only link to itself and is rejected by the loader.
CREATE TABLE IF NOT EXISTS `_backup_20260918_duplicate_kalecgos_dataset_action` LIKE `smart_scripts`;

INSERT IGNORE INTO `_backup_20260918_duplicate_kalecgos_dataset_action`
SELECT *
FROM `smart_scripts`
WHERE `entryorguid` = 38017
  AND `source_type` = 0
  AND `id` = 1
  AND `link` = 1
  AND `event_type` = 38
  AND `event_param1` = 0
  AND `event_param2` = 2
  AND `action_type` = 80
  AND `action_param1` = 3801701;

DELETE FROM `smart_scripts`
WHERE `entryorguid` = 38017
  AND `source_type` = 0
  AND `id` = 1
  AND `link` = 1
  AND `event_type` = 38
  AND `event_param1` = 0
  AND `event_param2` = 2
  AND `action_type` = 80
  AND `action_param1` = 3801701;
