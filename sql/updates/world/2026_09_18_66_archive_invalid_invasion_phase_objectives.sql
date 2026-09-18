-- These two upstream rows use negative QuestID/ObjectID sentinel values that
-- CONDITION_QUEST_OBJECTIVE_DONE has never supported. They are rejected during
-- condition loading and therefore cannot affect the phase transitions. Keep a
-- recoverable copy, then remove only the unreachable invalid rows.
CREATE TABLE IF NOT EXISTS `_backup_20260918_invalid_invasion_phase_objectives` LIKE `conditions`;

INSERT IGNORE INTO `_backup_20260918_invalid_invasion_phase_objectives`
SELECT *
FROM `conditions`
WHERE `SourceTypeOrReferenceId` = 23
  AND `SourceGroup` = 7541
  AND `SourceEntry` IN (142, 143)
  AND `ElseGroup` = 1
  AND `ConditionTypeOrReference` = 41
  AND `ConditionValue1` = -45406
  AND `ConditionValue2` = -1;

DELETE FROM `conditions`
WHERE `SourceTypeOrReferenceId` = 23
  AND `SourceGroup` = 7541
  AND `SourceEntry` IN (142, 143)
  AND `ElseGroup` = 1
  AND `ConditionTypeOrReference` = 41
  AND `ConditionValue1` = -45406
  AND `ConditionValue2` = -1;
