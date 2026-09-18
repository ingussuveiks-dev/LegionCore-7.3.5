-- Menu 17264 has options 1 (quest food dialogue) and 2 (vendor). The quest
-- condition for missing option 0 duplicates the valid condition on option 1.
CREATE TABLE IF NOT EXISTS `_backup_20260918_ooka_duplicate_condition` LIKE `conditions`;

INSERT IGNORE INTO `_backup_20260918_ooka_duplicate_condition`
SELECT *
FROM `conditions`
WHERE `SourceTypeOrReferenceId` = 15
  AND `SourceGroup` = 17264
  AND `SourceEntry` = 0
  AND `ConditionTypeOrReference` = 9
  AND `ConditionValue1` = 37536;

DELETE FROM `conditions`
WHERE `SourceTypeOrReferenceId` = 15
  AND `SourceGroup` = 17264
  AND `SourceEntry` = 0
  AND `ConditionTypeOrReference` = 9
  AND `ConditionValue1` = 37536;
