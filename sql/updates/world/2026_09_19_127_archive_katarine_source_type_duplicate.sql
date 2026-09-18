-- 9956200 is Katarine's timed action-list ID (source_type 9), not a creature
-- template. The stray source_type 0 visibility row duplicates Katarine 99562's
-- own 45-second visibility event and cannot be part of the timed list as ID 4.
CREATE TABLE IF NOT EXISTS `_backup_20260919_katarine_source_type_duplicate` LIKE `smart_scripts`;

INSERT IGNORE INTO `_backup_20260919_katarine_source_type_duplicate`
SELECT *
FROM `smart_scripts`
WHERE `entryorguid` = 9956200 AND `source_type` = 0 AND `id` = 4
  AND `action_type` = 47 AND `action_param1` = 1;

DELETE FROM `smart_scripts`
WHERE `entryorguid` = 9956200 AND `source_type` = 0 AND `id` = 4
  AND `action_type` = 47 AND `action_param1` = 1;
