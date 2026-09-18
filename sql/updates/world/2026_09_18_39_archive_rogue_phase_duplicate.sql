-- Phase 5287/1 already has the valid quest-none condition for quest 40847.
-- Archive the identical import duplicate carrying an unusable value3 flag.
CREATE TABLE IF NOT EXISTS `_backup_20260918_rogue_phase_duplicate` LIKE `conditions`;

INSERT IGNORE INTO `_backup_20260918_rogue_phase_duplicate`
SELECT *
FROM `conditions`
WHERE `SourceTypeOrReferenceId` = 23
  AND `SourceGroup` = 5287
  AND `SourceEntry` = 1
  AND `SourceId` = 0
  AND `ElseGroup` = 1
  AND `ConditionTypeOrReference` = 14
  AND `ConditionTarget` = 0
  AND `ConditionValue1` = 40847
  AND `ConditionValue2` = 0
  AND `ConditionValue3` = 1;

DELETE FROM `conditions`
WHERE `SourceTypeOrReferenceId` = 23
  AND `SourceGroup` = 5287
  AND `SourceEntry` = 1
  AND `SourceId` = 0
  AND `ElseGroup` = 1
  AND `ConditionTypeOrReference` = 14
  AND `ConditionTarget` = 0
  AND `ConditionValue1` = 40847
  AND `ConditionValue2` = 0
  AND `ConditionValue3` = 1;
