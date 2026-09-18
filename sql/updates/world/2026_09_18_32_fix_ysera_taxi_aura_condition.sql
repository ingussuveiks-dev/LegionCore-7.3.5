-- Spell 183851 has its only unit-owned aura at zero-based effect index 2.
-- Effect index 3 is a trigger effect and cannot be queried as an AuraEffect.
CREATE TABLE IF NOT EXISTS `_backup_20260918_ysera_taxi_aura_condition` LIKE `conditions`;

INSERT IGNORE INTO `_backup_20260918_ysera_taxi_aura_condition`
SELECT *
FROM `conditions`
WHERE `SourceTypeOrReferenceId` = 23
  AND `SourceGroup` = 7558
  AND `SourceEntry` = 28
  AND `SourceId` = 0
  AND `ElseGroup` = 0
  AND `ConditionTypeOrReference` = 1
  AND `ConditionTarget` = 0
  AND `ConditionValue1` = 183851
  AND `ConditionValue2` = 3
  AND `ConditionValue3` = 0;

UPDATE `conditions`
SET `ConditionValue2` = 2
WHERE `SourceTypeOrReferenceId` = 23
  AND `SourceGroup` = 7558
  AND `SourceEntry` = 28
  AND `SourceId` = 0
  AND `ElseGroup` = 0
  AND `ConditionTypeOrReference` = 1
  AND `ConditionTarget` = 0
  AND `ConditionValue1` = 183851
  AND `ConditionValue2` = 3
  AND `ConditionValue3` = 0;
