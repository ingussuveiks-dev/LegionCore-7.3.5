-- SMART_EVENT_FRIENDLY_MISSING_BUFF requires a non-zero search radius. Preserve
-- the row before matching the one-yard self-check pattern used by other SAI buffs.
CREATE TABLE IF NOT EXISTS `_backup_20260918_warsong_wing_commander_buff_radius` LIKE `smart_scripts`;

INSERT IGNORE INTO `_backup_20260918_warsong_wing_commander_buff_radius`
SELECT *
FROM `smart_scripts`
WHERE `entryorguid` = 40942 AND `source_type` = 0 AND `id` = 0
  AND `event_type` = 16 AND `event_param1` = 46598 AND `event_param2` = 0;

UPDATE `smart_scripts`
SET `event_param2` = 1
WHERE `entryorguid` = 40942 AND `source_type` = 0 AND `id` = 0
  AND `event_type` = 16 AND `event_param1` = 46598 AND `event_param2` = 0;
