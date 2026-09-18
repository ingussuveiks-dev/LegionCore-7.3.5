-- Reference 228138 is reached only through Legion world loot and is processed
-- by ProcessWorld, which chooses one eligible group member with equal odds.
-- Declare the twelve shoulder-enchant bonus containers as an equal-chance
-- group instead of twelve misleading 100% entries (reported as 1200%).
CREATE TABLE IF NOT EXISTS `_backup_20260918_world_bonus_reference_chances` LIKE `reference_loot_template`;

INSERT IGNORE INTO `_backup_20260918_world_bonus_reference_chances`
SELECT *
FROM `reference_loot_template`
WHERE `Entry` = 228138
  AND `Item` IN (140220, 140221, 140222, 140224, 140225, 140226, 140227, 142259, 144330, 144345, 153202, 153248)
  AND `Currency` = 0
  AND `Reference` = 0
  AND `Chance` = 100
  AND `QuestRequired` = 0
  AND `LootMode` = 0
  AND `GroupId` = 1
  AND `MinCount` = 1
  AND `MaxCount` = 1;

UPDATE `reference_loot_template`
SET `Chance` = 0,
    `Comment` = 'Equal-chance Legion world shoulder-enchant bonus container'
WHERE `Entry` = 228138
  AND `Item` IN (140220, 140221, 140222, 140224, 140225, 140226, 140227, 142259, 144330, 144345, 153202, 153248)
  AND `Currency` = 0
  AND `Reference` = 0
  AND `Chance` = 100
  AND `QuestRequired` = 0
  AND `LootMode` = 0
  AND `GroupId` = 1
  AND `MinCount` = 1
  AND `MaxCount` = 1;
