-- Each invalid type-0 row has an exact, valid CONDITION_AURA (type 1) twin in
-- the same SmartAI condition group. Archive only the rejected duplicates.
CREATE TABLE IF NOT EXISTS `_backup_20260918_teron_aura_duplicates` LIKE `conditions`;

INSERT IGNORE INTO `_backup_20260918_teron_aura_duplicates`
SELECT *
FROM `conditions`
WHERE `SourceTypeOrReferenceId` = 22
  AND `SourceGroup` = 2
  AND `SourceEntry` = 231022
  AND `SourceId` = 0
  AND `ElseGroup` = 1
  AND `ConditionTypeOrReference` = 0
  AND `ConditionTarget` = 0
  AND `ConditionValue1` IN (182164, 182166, 182167)
  AND `ConditionValue2` = 0
  AND `ConditionValue3` = 0
  AND `NegativeCondition` = 1;

DELETE FROM `conditions`
WHERE `SourceTypeOrReferenceId` = 22
  AND `SourceGroup` = 2
  AND `SourceEntry` = 231022
  AND `SourceId` = 0
  AND `ElseGroup` = 1
  AND `ConditionTypeOrReference` = 0
  AND `ConditionTarget` = 0
  AND `ConditionValue1` IN (182164, 182166, 182167)
  AND `ConditionValue2` = 0
  AND `ConditionValue3` = 0
  AND `NegativeCondition` = 1;
