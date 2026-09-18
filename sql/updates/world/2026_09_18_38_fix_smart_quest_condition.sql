-- CONDITION_QUESTTAKEN uses only value1 (quest ID). The legacy boolean in
-- value2 is rejected as useless and does not change the intended "has quest".
CREATE TABLE IF NOT EXISTS `_backup_20260918_smart_quest_condition` LIKE `conditions`;

INSERT IGNORE INTO `_backup_20260918_smart_quest_condition`
SELECT *
FROM `conditions`
WHERE `SourceTypeOrReferenceId` = 22
  AND `SourceGroup` = 1
  AND `SourceEntry` = 600
  AND `SourceId` = 10
  AND `ElseGroup` = 0
  AND `ConditionTypeOrReference` = 9
  AND `ConditionTarget` = 0
  AND `ConditionValue1` = 34429
  AND `ConditionValue2` = 1
  AND `ConditionValue3` = 0;

UPDATE `conditions`
SET `ConditionValue2` = 0
WHERE `SourceTypeOrReferenceId` = 22
  AND `SourceGroup` = 1
  AND `SourceEntry` = 600
  AND `SourceId` = 10
  AND `ElseGroup` = 0
  AND `ConditionTypeOrReference` = 9
  AND `ConditionTarget` = 0
  AND `ConditionValue1` = 34429
  AND `ConditionValue2` = 1
  AND `ConditionValue3` = 0;
