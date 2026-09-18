-- Roakk's Set Home Position row had SMART_EVENT_AGGRO (4) shifted into
-- event_param1, leaving event_type as UPDATE_IC and an invalid 4..0 timer.
CREATE TABLE IF NOT EXISTS `_backup_20260918_roakk_home_event` LIKE `smart_scripts`;

INSERT IGNORE INTO `_backup_20260918_roakk_home_event`
SELECT *
FROM `smart_scripts`
WHERE `entryorguid` = 79490
  AND `source_type` = 0
  AND `id` = 7
  AND `link` = 0;

UPDATE `smart_scripts`
SET `event_type` = 4,
    `event_param1` = 0,
    `comment` = 'Roakk the Zealot - On Aggro - Set Home Position'
WHERE `entryorguid` = 79490
  AND `source_type` = 0
  AND `id` = 7
  AND `link` = 0
  AND `event_type` = 0
  AND `event_param1` = 4
  AND `event_param2` = 0
  AND `action_type` = 101;
