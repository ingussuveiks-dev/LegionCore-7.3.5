-- Enfilade uses the same two rewarded-quest prerequisites as The Master
-- Siegesmith, but both imported rows lost CONDITION_QUESTREWARDED (type 8).
CREATE TABLE IF NOT EXISTS `_backup_20260918_enfilade_prerequisites` LIKE `conditions`;

INSERT IGNORE INTO `_backup_20260918_enfilade_prerequisites`
SELECT *
FROM `conditions`
WHERE `SourceTypeOrReferenceId` = 19
  AND `SourceGroup` = 0
  AND `SourceEntry` = 32994
  AND `SourceId` = 0
  AND `ElseGroup` = 0
  AND `ConditionTypeOrReference` = 0
  AND `ConditionTarget` = 0
  AND `ConditionValue1` IN (33785, 33826)
  AND `ConditionValue2` = 0
  AND `ConditionValue3` = 0;

UPDATE `conditions`
SET `ConditionTypeOrReference` = 8
WHERE `SourceTypeOrReferenceId` = 19
  AND `SourceGroup` = 0
  AND `SourceEntry` = 32994
  AND `SourceId` = 0
  AND `ElseGroup` = 0
  AND `ConditionTypeOrReference` = 0
  AND `ConditionTarget` = 0
  AND `ConditionValue1` IN (33785, 33826)
  AND `ConditionValue2` = 0
  AND `ConditionValue3` = 0;
