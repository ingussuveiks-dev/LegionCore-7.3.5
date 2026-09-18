-- Fjorlin's second quest option lost the quest ID, while its comment and the
-- sibling option both identify Pushed Too Far (12869).
CREATE TABLE IF NOT EXISTS `_backup_20260918_fjorlin_gossip_quest` LIKE `conditions`;

INSERT IGNORE INTO `_backup_20260918_fjorlin_gossip_quest`
SELECT *
FROM `conditions`
WHERE `SourceTypeOrReferenceId` = 15
  AND `SourceGroup` = 9891
  AND `SourceEntry` = 1
  AND `SourceId` = 0
  AND `ElseGroup` = 0
  AND `ConditionTypeOrReference` = 9
  AND `ConditionTarget` = 0
  AND `ConditionValue1` = 0
  AND `ConditionValue2` = 0
  AND `ConditionValue3` = 0;

UPDATE `conditions`
SET `ConditionValue1` = 12869
WHERE `SourceTypeOrReferenceId` = 15
  AND `SourceGroup` = 9891
  AND `SourceEntry` = 1
  AND `SourceId` = 0
  AND `ElseGroup` = 0
  AND `ConditionTypeOrReference` = 9
  AND `ConditionTarget` = 0
  AND `ConditionValue1` = 0
  AND `ConditionValue2` = 0
  AND `ConditionValue3` = 0;
