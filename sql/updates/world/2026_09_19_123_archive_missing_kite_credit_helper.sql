-- Creature template 68286, an old timed helper for quest 32351, is absent from
-- the current database. The coincidentally numbered spawn GUID 68286 belongs to
-- Converted Hero (32255) and must not be treated as this script's owner.
CREATE TABLE IF NOT EXISTS `_backup_20260919_missing_kite_credit_helper` LIKE `smart_scripts`;

INSERT IGNORE INTO `_backup_20260919_missing_kite_credit_helper`
SELECT *
FROM `smart_scripts`
WHERE `entryorguid` = 68286 AND `source_type` = 0 AND `id` = 0
  AND `action_type` = 33 AND `action_param1` = 68723;

DELETE FROM `smart_scripts`
WHERE `entryorguid` = 68286 AND `source_type` = 0 AND `id` = 0
  AND `action_type` = 33 AND `action_param1` = 68723;
