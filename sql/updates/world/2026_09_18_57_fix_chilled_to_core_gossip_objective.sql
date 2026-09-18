-- This 7.3.5 world uses the original Archmage Cedric entry 18165 for quest
-- 45846 and menu 7593. Entry 125016 has no creature template or spawn here.
CREATE TABLE IF NOT EXISTS `_backup_20260918_chilled_to_core_condition` LIKE `conditions`;

INSERT IGNORE INTO `_backup_20260918_chilled_to_core_condition`
SELECT *
FROM `conditions`
WHERE `SourceTypeOrReferenceId` = 15
  AND `SourceGroup` = 7593
  AND `SourceEntry` = 0
  AND `ConditionTypeOrReference` = 41
  AND `ConditionValue1` = 45846
  AND `ConditionValue2` = 125016;

UPDATE `conditions`
SET `ConditionValue2` = 18165
WHERE `SourceTypeOrReferenceId` = 15
  AND `SourceGroup` = 7593
  AND `SourceEntry` = 0
  AND `ConditionTypeOrReference` = 41
  AND `ConditionValue1` = 45846
  AND `ConditionValue2` = 125016;
