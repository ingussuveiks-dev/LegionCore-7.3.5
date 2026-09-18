-- SetData(1,1) should award the return credit, move the captured murloc to
-- phase mask 2, and despawn it. The latter two actions had colliding ids and
-- the phase action was incorrectly stored as a second SetData self-link.
CREATE TABLE IF NOT EXISTS `_backup_20260918_captured_vile_fin_return_chain` LIKE `smart_scripts`;

INSERT IGNORE INTO `_backup_20260918_captured_vile_fin_return_chain`
SELECT *
FROM `smart_scripts`
WHERE `entryorguid` = 38923
  AND `source_type` = 0
  AND `id` IN (0, 1);

UPDATE `smart_scripts`
SET `link` = 2,
    `event_type` = 61,
    `event_param1` = 0,
    `event_param2` = 0
WHERE `entryorguid` = 38923
  AND `source_type` = 0
  AND `id` = 1
  AND `link` = 1
  AND `event_type` = 38
  AND `event_param1` = 1
  AND `event_param2` = 1
  AND `action_type` = 44
  AND `action_param1` = 2;

UPDATE `smart_scripts`
SET `id` = 2
WHERE `entryorguid` = 38923
  AND `source_type` = 0
  AND `id` = 1
  AND `link` = 0
  AND `event_type` = 61
  AND `action_type` = 41;
