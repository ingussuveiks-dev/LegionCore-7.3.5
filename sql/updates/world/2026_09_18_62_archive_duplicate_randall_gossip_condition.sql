-- Menu 20426 contains only option 0, already guarded by quest 45413 and its
-- incomplete credit. The quest condition on missing option 1 is redundant.
CREATE TABLE IF NOT EXISTS `_backup_20260918_randall_duplicate_condition` LIKE `conditions`;

INSERT IGNORE INTO `_backup_20260918_randall_duplicate_condition`
SELECT *
FROM `conditions`
WHERE `SourceTypeOrReferenceId` = 15
  AND `SourceGroup` = 20426
  AND `SourceEntry` = 1
  AND `ConditionTypeOrReference` = 9
  AND `ConditionValue1` = 45413;

DELETE FROM `conditions`
WHERE `SourceTypeOrReferenceId` = 15
  AND `SourceGroup` = 20426
  AND `SourceEntry` = 1
  AND `ConditionTypeOrReference` = 9
  AND `ConditionValue1` = 45413;
