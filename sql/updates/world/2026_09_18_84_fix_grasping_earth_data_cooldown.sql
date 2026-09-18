-- Grasping Earth despawns on SetData(1, 1). Its repeat window was stored as
-- min=1, max=0, so SmartAI rejected the handler before it could run.
CREATE TABLE IF NOT EXISTS `_backup_20260918_grasping_earth_data_cooldown` LIKE `smart_scripts`;

INSERT IGNORE INTO `_backup_20260918_grasping_earth_data_cooldown`
SELECT *
FROM `smart_scripts`
WHERE `entryorguid` = 77893
  AND `source_type` = 0
  AND `id` = 0
  AND `link` = 0;

UPDATE `smart_scripts`
SET `event_param4` = 1,
    `comment` = 'Grasping Earth - On Data Set 1 1 - Despawn'
WHERE `entryorguid` = 77893
  AND `source_type` = 0
  AND `id` = 0
  AND `link` = 0
  AND `event_type` = 38
  AND `event_param1` = 1
  AND `event_param2` = 1
  AND `event_param3` = 1
  AND `event_param4` = 0
  AND `action_type` = 41;
