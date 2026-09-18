-- These Coordinator scripts distinguish capital-area spawns for their hand/dance
-- visual. AreaTable 7.3.5 marks 6457 and 5287 as child areas, so they must be
-- checked with CONDITION_AREAID rather than CONDITION_ZONEID.
CREATE TABLE IF NOT EXISTS `_backup_20260918_coordinator_area_conditions` LIKE `conditions`;

INSERT IGNORE INTO `_backup_20260918_coordinator_area_conditions`
SELECT *
FROM `conditions`
WHERE `SourceTypeOrReferenceId` = 22
  AND `SourceId` = 0
  AND `ElseGroup` = 0
  AND `ConditionTypeOrReference` = 4
  AND `ConditionTarget` = 0
  AND `ConditionValue2` = 0
  AND `ConditionValue3` = 0
  AND ((`SourceGroup` = 1 AND `SourceEntry` = 123984 AND `ConditionValue1` = 6457)
    OR (`SourceGroup` = 2 AND `SourceEntry` = 123991 AND `ConditionValue1` = 6457)
    OR (`SourceGroup` = 1 AND `SourceEntry` = 124246 AND `ConditionValue1` = 5287));

UPDATE `conditions`
SET `ConditionTypeOrReference` = 23
WHERE `SourceTypeOrReferenceId` = 22
  AND `SourceId` = 0
  AND `ElseGroup` = 0
  AND `ConditionTypeOrReference` = 4
  AND `ConditionTarget` = 0
  AND `ConditionValue2` = 0
  AND `ConditionValue3` = 0
  AND ((`SourceGroup` = 1 AND `SourceEntry` = 123984 AND `ConditionValue1` = 6457)
    OR (`SourceGroup` = 2 AND `SourceEntry` = 123991 AND `ConditionValue1` = 6457)
    OR (`SourceGroup` = 1 AND `SourceEntry` = 124246 AND `ConditionValue1` = 5287));
