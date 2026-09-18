-- These rows use the old two-parameter distance layout (distance, comparison).
-- In this core CONDITION_DISTANCE_TO uses (other target, distance, comparison).
CREATE TABLE IF NOT EXISTS `_backup_20260918_spell_distance_conditions` LIKE `conditions`;

INSERT IGNORE INTO `_backup_20260918_spell_distance_conditions`
SELECT *
FROM `conditions`
WHERE `SourceTypeOrReferenceId` = 13
  AND `SourceGroup` = 1
  AND `SourceEntry` IN (181555, 214176)
  AND `ConditionTypeOrReference` = 35
  AND `ConditionTarget` = 0
  AND `ConditionValue1` = 10
  AND `ConditionValue2` = 4
  AND `ConditionValue3` = 0;

UPDATE `conditions`
SET `ConditionValue1` = 1,
    `ConditionValue2` = 10,
    `ConditionValue3` = 4
WHERE `SourceTypeOrReferenceId` = 13
  AND `SourceGroup` = 1
  AND `SourceEntry` IN (181555, 214176)
  AND `ConditionTypeOrReference` = 35
  AND `ConditionTarget` = 0
  AND `ConditionValue1` = 10
  AND `ConditionValue2` = 4
  AND `ConditionValue3` = 0;
