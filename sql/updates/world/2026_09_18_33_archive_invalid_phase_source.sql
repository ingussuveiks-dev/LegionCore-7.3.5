-- The valid phase-definition condition already exists with source type 23.
-- Archive the identical import duplicate whose source type 41 is unsupported.
CREATE TABLE IF NOT EXISTS `_backup_20260918_invalid_phase_source` LIKE `conditions`;

INSERT IGNORE INTO `_backup_20260918_invalid_phase_source`
SELECT *
FROM `conditions`
WHERE `SourceTypeOrReferenceId` = 41
  AND `SourceGroup` = 1519
  AND `SourceEntry` = 8
  AND `SourceId` = 0
  AND `ElseGroup` = 1
  AND `ConditionTypeOrReference` = 14
  AND `ConditionTarget` = 0
  AND `ConditionValue1` = 42740
  AND `ConditionValue2` = 0
  AND `ConditionValue3` = 0
  AND `NegativeCondition` = 1;

DELETE FROM `conditions`
WHERE `SourceTypeOrReferenceId` = 41
  AND `SourceGroup` = 1519
  AND `SourceEntry` = 8
  AND `SourceId` = 0
  AND `ElseGroup` = 1
  AND `ConditionTypeOrReference` = 14
  AND `ConditionTarget` = 0
  AND `ConditionValue1` = 42740
  AND `ConditionValue2` = 0
  AND `ConditionValue3` = 0
  AND `NegativeCondition` = 1;
