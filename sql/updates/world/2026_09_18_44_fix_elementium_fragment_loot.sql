-- Essence of Destruction and Elementium Gem Cluster are independent Dragon
-- Soul chest drops. Their shared group incorrectly made them alternatives and
-- produced a 140% group total.
CREATE TABLE IF NOT EXISTS `_backup_20260918_elementium_fragment_loot` LIKE `gameobject_loot_template`;

INSERT IGNORE INTO `_backup_20260918_elementium_fragment_loot`
SELECT *
FROM `gameobject_loot_template`
WHERE `Entry` = 210220
  AND `Item` IN (71998, 77952)
  AND `Currency` = 0
  AND `Reference` = 0
  AND `Chance` = 70
  AND `QuestRequired` = 0
  AND `LootMode` = 15
  AND `GroupId` = 2;

UPDATE `gameobject_loot_template`
SET `GroupId` = 0
WHERE `Entry` = 210220
  AND `Item` IN (71998, 77952)
  AND `Currency` = 0
  AND `Reference` = 0
  AND `Chance` = 70
  AND `QuestRequired` = 0
  AND `LootMode` = 15
  AND `GroupId` = 2;
