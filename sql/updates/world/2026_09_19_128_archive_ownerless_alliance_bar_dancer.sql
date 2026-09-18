-- The large positive value resembles a spawn GUID, but neither GUID 14677644
-- nor creature template 14677644 exists. The same ownerless row is present in
-- both reference dumps, so no dancer NPC can be identified safely.
CREATE TABLE IF NOT EXISTS `_backup_20260919_ownerless_alliance_bar_dancer` LIKE `smart_scripts`;

INSERT IGNORE INTO `_backup_20260919_ownerless_alliance_bar_dancer`
SELECT *
FROM `smart_scripts`
WHERE `entryorguid` = 14677644 AND `source_type` = 0 AND `id` = 0
  AND `action_type` = 5 AND `action_param1` = 400
  AND `comment` = 'Dancer Alliance Bar';

DELETE FROM `smart_scripts`
WHERE `entryorguid` = 14677644 AND `source_type` = 0 AND `id` = 0
  AND `action_type` = 5 AND `action_param1` = 400
  AND `comment` = 'Dancer Alliance Bar';
