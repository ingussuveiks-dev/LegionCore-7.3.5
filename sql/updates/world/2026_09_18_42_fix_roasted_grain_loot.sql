-- 138623 is the item "Sack of Roasted Grain" in the 7.3.5 ItemSparse data,
-- not a currency. Restore the shifted loot field instead of merely removing
-- the group that made the invalid currency row fail validation.
CREATE TABLE IF NOT EXISTS `_backup_20260918_roasted_grain_loot` LIKE `gameobject_loot_template`;

INSERT IGNORE INTO `_backup_20260918_roasted_grain_loot`
SELECT *
FROM `gameobject_loot_template`
WHERE `Entry` = 40870
  AND `Item` = 0
  AND `Currency` = 138623
  AND `Reference` = 0
  AND `Chance` = 100
  AND `QuestRequired` = 0
  AND `LootMode` = 1
  AND `GroupId` = 1
  AND `MinCount` = 1
  AND `MaxCount` = 1;

UPDATE `gameobject_loot_template`
SET `Item` = 138623,
    `Currency` = 0
WHERE `Entry` = 40870
  AND `Item` = 0
  AND `Currency` = 138623
  AND `Reference` = 0
  AND `Chance` = 100
  AND `QuestRequired` = 0
  AND `LootMode` = 1
  AND `GroupId` = 1
  AND `MinCount` = 1
  AND `MaxCount` = 1;
