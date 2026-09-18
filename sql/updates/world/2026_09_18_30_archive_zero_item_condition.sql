-- Professor Pallin gossip option 1 already has the valid requirement for 10 of
-- item 129092. Archive the duplicate imported row whose impossible count of 0
-- is rejected by ConditionMgr.
CREATE TABLE IF NOT EXISTS `_backup_20260918_zero_item_condition` LIKE `conditions`;

INSERT IGNORE INTO `_backup_20260918_zero_item_condition`
SELECT *
FROM `conditions`
WHERE `SourceTypeOrReferenceId` = 15
  AND `SourceGroup` = 18598
  AND `SourceEntry` = 1
  AND `SourceId` = 0
  AND `ElseGroup` = 0
  AND `ConditionTypeOrReference` = 2
  AND `ConditionTarget` = 0
  AND `ConditionValue1` = 129092
  AND `ConditionValue2` = 0
  AND `ConditionValue3` = 0;

DELETE FROM `conditions`
WHERE `SourceTypeOrReferenceId` = 15
  AND `SourceGroup` = 18598
  AND `SourceEntry` = 1
  AND `SourceId` = 0
  AND `ElseGroup` = 0
  AND `ConditionTypeOrReference` = 2
  AND `ConditionTarget` = 0
  AND `ConditionValue1` = 129092
  AND `ConditionValue2` = 0
  AND `ConditionValue3` = 0;
