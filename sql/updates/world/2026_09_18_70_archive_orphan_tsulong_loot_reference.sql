-- LegionCore's 2024 base dump added this 1% reference to Cache of Tsulong,
-- but neither that dump nor the older 2020 dump contains reference loot 86279.
-- The cache already has its complete difficulty-specific direct loot rows, so
-- the unresolved reference can never yield an item. Preserve it for recovery
-- and remove only the invalid, unusable row from the active loot template.
CREATE TABLE IF NOT EXISTS `_backup_20260918_orphan_tsulong_loot_reference` LIKE `gameobject_loot_template`;

INSERT IGNORE INTO `_backup_20260918_orphan_tsulong_loot_reference`
SELECT *
FROM `gameobject_loot_template`
WHERE `Entry` = 43568
  AND `Item` = 1
  AND `Reference` = 86279
  AND `Chance` = 1
  AND `QuestRequired` = 0
  AND `LootMode` = 31
  AND `GroupId` = 0
  AND `MinCount` = 1
  AND `MaxCount` = 1;

DELETE FROM `gameobject_loot_template`
WHERE `Entry` = 43568
  AND `Item` = 1
  AND `Reference` = 86279
  AND `Chance` = 1
  AND `QuestRequired` = 0
  AND `LootMode` = 31
  AND `GroupId` = 0
  AND `MinCount` = 1
  AND `MaxCount` = 1;
