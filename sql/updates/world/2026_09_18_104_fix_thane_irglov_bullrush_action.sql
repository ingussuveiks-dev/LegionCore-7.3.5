-- Thane Irglov's Bull Rush row has the normal SMART_ACTION_CAST layout
-- (spell in param1 and victim target), but its action type was an undefined 255.
CREATE TABLE IF NOT EXISTS `_backup_20260918_thane_irglov_bullrush_action` LIKE `smart_scripts`;

INSERT IGNORE INTO `_backup_20260918_thane_irglov_bullrush_action`
SELECT *
FROM `smart_scripts`
WHERE `entryorguid` = 91892
  AND `source_type` = 0
  AND `id` = 1
  AND `link` = 0;

UPDATE `smart_scripts`
SET `action_type` = 11,
    `comment` = 'Thane Irglov the Merciless - In Combat - Cast Bull Rush'
WHERE `entryorguid` = 91892
  AND `source_type` = 0
  AND `id` = 1
  AND `link` = 0
  AND `event_type` = 0
  AND `action_type` = 255
  AND `action_param1` = 187406
  AND `target_type` = 2;
