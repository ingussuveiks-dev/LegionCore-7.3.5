-- Phase 7543 is active before quest 42053 objective 107166 is done; phase
-- 7544 is its completed-objective successor. 112549 belongs to another zone.
CREATE TABLE IF NOT EXISTS `_backup_20260918_yvelyn_phase_condition` LIKE `conditions`;

INSERT IGNORE INTO `_backup_20260918_yvelyn_phase_condition`
SELECT *
FROM `conditions`
WHERE `SourceTypeOrReferenceId` = 23
  AND `SourceGroup` = 8180
  AND `SourceEntry` = 3
  AND `ConditionTypeOrReference` = 41
  AND `ConditionValue1` = 42053
  AND `ConditionValue2` = 112549;

UPDATE `conditions`
SET `ConditionValue2` = 107166
WHERE `SourceTypeOrReferenceId` = 23
  AND `SourceGroup` = 8180
  AND `SourceEntry` = 3
  AND `ConditionTypeOrReference` = 41
  AND `ConditionValue1` = 42053
  AND `ConditionValue2` = 112549;
