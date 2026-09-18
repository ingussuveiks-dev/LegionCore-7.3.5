-- Agatha's first movement step has two meaningful delay bounds in reverse
-- order. Preserve both values and normalize the random delay to 1-2 seconds.
CREATE TABLE IF NOT EXISTS `_backup_20260918_agatha_movement_timer` LIKE `smart_scripts`;

INSERT IGNORE INTO `_backup_20260918_agatha_movement_timer`
SELECT *
FROM `smart_scripts`
WHERE `entryorguid` = 4460800
  AND `source_type` = 9
  AND `id` = 0
  AND `link` = 0;

UPDATE `smart_scripts`
SET `event_param1` = 1000,
    `event_param2` = 2000,
    `comment` = 'Agatha Sequence - After 1-2 Seconds - Move to First Position'
WHERE `entryorguid` = 4460800
  AND `source_type` = 9
  AND `id` = 0
  AND `link` = 0
  AND `event_type` = 0
  AND `event_param1` = 2000
  AND `event_param2` = 1000
  AND `action_type` = 69
  AND `target_type` = 8;
