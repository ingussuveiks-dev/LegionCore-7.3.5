-- CONDITION_QUESTTAKEN uses only value1 (quest ID). These rows also contain
-- stale objective/boolean fields already covered by separate conditions.
CREATE TABLE IF NOT EXISTS `_backup_20260918_active_quest_conditions` LIKE `conditions`;

INSERT IGNORE INTO `_backup_20260918_active_quest_conditions`
SELECT *
FROM `conditions`
WHERE (`SourceTypeOrReferenceId` = 15
       AND `SourceGroup` = 19908
       AND `SourceEntry` = 0
       AND `SourceId` = 0
       AND `ElseGroup` = 0
       AND `ConditionTypeOrReference` = 9
       AND `ConditionValue1` = 42833
       AND `ConditionValue2` = 109241
       AND `ConditionValue3` = 0)
   OR (`SourceTypeOrReferenceId` = 22
       AND `SourceGroup` = 2
       AND `SourceEntry` = 231013
       AND `SourceId` = 0
       AND `ElseGroup` = 0
       AND `ConditionTypeOrReference` = 9
       AND `ConditionValue1` = 33837
       AND `ConditionValue2` = 82283
       AND `ConditionValue3` = 1);

UPDATE `conditions`
SET `ConditionValue2` = 0,
    `ConditionValue3` = 0
WHERE (`SourceTypeOrReferenceId` = 15
       AND `SourceGroup` = 19908
       AND `SourceEntry` = 0
       AND `SourceId` = 0
       AND `ElseGroup` = 0
       AND `ConditionTypeOrReference` = 9
       AND `ConditionValue1` = 42833
       AND `ConditionValue2` = 109241
       AND `ConditionValue3` = 0)
   OR (`SourceTypeOrReferenceId` = 22
       AND `SourceGroup` = 2
       AND `SourceEntry` = 231013
       AND `SourceId` = 0
       AND `ElseGroup` = 0
       AND `ConditionTypeOrReference` = 9
       AND `ConditionValue1` = 33837
       AND `ConditionValue2` = 82283
       AND `ConditionValue3` = 1);
