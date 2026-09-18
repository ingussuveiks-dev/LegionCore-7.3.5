-- Gameobject 230253 (Glowing Obsidian Shard) has no template or spawn in this
-- database. The treasure was not independently obtainable in retail WoD; its
-- quest flag 34521 was completed by another treasure. GUID 230253 is instead an
-- unrelated Jeweled Macaw creature and is not a replacement script source.
CREATE TABLE IF NOT EXISTS `_backup_20260919_unused_obsidian_shard_script` LIKE `smart_scripts`;

INSERT IGNORE INTO `_backup_20260919_unused_obsidian_shard_script`
SELECT *
FROM `smart_scripts`
WHERE `entryorguid` = 230253 AND `source_type` = 1 AND `id` = 0
  AND `event_type` = 70 AND `action_type` = 7 AND `action_param1` = 34521;

DELETE FROM `smart_scripts`
WHERE `entryorguid` = 230253 AND `source_type` = 1 AND `id` = 0
  AND `event_type` = 70 AND `action_type` = 7 AND `action_param1` = 34521;
