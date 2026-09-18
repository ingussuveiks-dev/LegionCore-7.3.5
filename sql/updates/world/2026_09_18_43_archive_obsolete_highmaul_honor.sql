-- Currency 392 was the pre-Legion Honor Points currency. It is absent from
-- CurrencyTypes in client build 7.3.5.26972, so this old Highmaul Lockbox
-- reward cannot be granted by this game version. Preserve the complete row.
CREATE TABLE IF NOT EXISTS `_backup_20260918_obsolete_highmaul_honor` LIKE `item_loot_template`;

INSERT IGNORE INTO `_backup_20260918_obsolete_highmaul_honor`
SELECT *
FROM `item_loot_template`
WHERE `Entry` = 119000
  AND `Item` = 0
  AND `Currency` = 392
  AND `Reference` = 0
  AND `Chance` = 4
  AND `QuestRequired` = 0
  AND `LootMode` = 1
  AND `GroupId` = 0
  AND `MinCount` = 23
  AND `MaxCount` = 23;

DELETE FROM `item_loot_template`
WHERE `Entry` = 119000
  AND `Item` = 0
  AND `Currency` = 392
  AND `Reference` = 0
  AND `Chance` = 4
  AND `QuestRequired` = 0
  AND `LootMode` = 1
  AND `GroupId` = 0
  AND `MinCount` = 23
  AND `MaxCount` = 23;
