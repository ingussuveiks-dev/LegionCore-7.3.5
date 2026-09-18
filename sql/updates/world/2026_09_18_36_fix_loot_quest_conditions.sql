-- Three loot-item conditions have the quest ID shifted from value1 to value2.
-- Restore the CONDITION_QUESTREWARDED parameter layout (quest, 0, 0).
CREATE TABLE IF NOT EXISTS `_backup_20260918_loot_quest_conditions` LIKE `conditions`;

INSERT IGNORE INTO `_backup_20260918_loot_quest_conditions`
SELECT *
FROM `conditions`
WHERE `SourceTypeOrReferenceId` = 30
  AND `SourceGroup` = 1
  AND `SourceEntry` IN (129747, 129928, 143776)
  AND `SourceId` = 0
  AND `ElseGroup` = 0
  AND `ConditionTypeOrReference` = 8
  AND `ConditionTarget` = 0
  AND `ConditionValue1` = 0
  AND `ConditionValue2` IN (40168, 40173, 45563)
  AND `ConditionValue3` = 0
  AND `NegativeCondition` = 1;

UPDATE `conditions`
SET `ConditionValue1` = `ConditionValue2`,
    `ConditionValue2` = 0
WHERE `SourceTypeOrReferenceId` = 30
  AND `SourceGroup` = 1
  AND `SourceEntry` IN (129747, 129928, 143776)
  AND `SourceId` = 0
  AND `ElseGroup` = 0
  AND `ConditionTypeOrReference` = 8
  AND `ConditionTarget` = 0
  AND `ConditionValue1` = 0
  AND `ConditionValue2` IN (40168, 40173, 45563)
  AND `ConditionValue3` = 0
  AND `NegativeCondition` = 1;
