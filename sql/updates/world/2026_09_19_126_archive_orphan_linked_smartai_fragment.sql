-- This row has existed as an isolated linked-action fragment in both the 2020
-- and 2024 reference databases. There is no creature template/spawn 395280 and
-- no parent event row that can link to ID 10, so its intended owner is unknown.
CREATE TABLE IF NOT EXISTS `_backup_20260919_orphan_linked_smartai_fragment` LIKE `smart_scripts`;

INSERT IGNORE INTO `_backup_20260919_orphan_linked_smartai_fragment`
SELECT *
FROM `smart_scripts`
WHERE `entryorguid` = 395280 AND `source_type` = 0 AND `id` = 10
  AND `link` = 11 AND `event_type` = 61 AND `action_type` = 18
  AND `action_param1` = 768;

DELETE FROM `smart_scripts`
WHERE `entryorguid` = 395280 AND `source_type` = 0 AND `id` = 10
  AND `link` = 11 AND `event_type` = 61 AND `action_type` = 18
  AND `action_param1` = 768;
