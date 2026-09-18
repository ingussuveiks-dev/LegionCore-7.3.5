-- Creature credit 96255 belongs to quest 39590, not the item-only quest 39595.
-- The condition guards the linked conversation and its own comment says it
-- should run while the player has the quest, so use QUEST_TAKEN explicitly.
CREATE TABLE IF NOT EXISTS `_backup_20260918_missing_vrykul_champion_condition` LIKE `conditions`;

INSERT IGNORE INTO `_backup_20260918_missing_vrykul_champion_condition`
SELECT *
FROM `conditions`
WHERE `SourceTypeOrReferenceId` = 22
  AND `SourceGroup` = 1
  AND `SourceEntry` = 96255
  AND `SourceId` = 0
  AND `ConditionTypeOrReference` = 41
  AND `ConditionValue1` = 39595
  AND `ConditionValue2` = 96255;

UPDATE `conditions`
SET `ConditionTypeOrReference` = 9,
    `ConditionValue1` = 39590,
    `ConditionValue2` = 0,
    `ConditionValue3` = 0,
    `NegativeCondition` = 0,
    `Comment` = 'SAI - Play conversation only while player has quest 39590'
WHERE `SourceTypeOrReferenceId` = 22
  AND `SourceGroup` = 1
  AND `SourceEntry` = 96255
  AND `SourceId` = 0
  AND `ConditionTypeOrReference` = 41
  AND `ConditionValue1` = 39595
  AND `ConditionValue2` = 96255;
