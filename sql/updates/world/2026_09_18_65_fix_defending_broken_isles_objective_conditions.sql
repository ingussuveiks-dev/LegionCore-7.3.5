-- CONDITION_QUEST_OBJECTIVE_DONE stores QuestID in value1 and the objective's
-- ObjectID in value2. These rows had the ObjectID shifted into value1.
CREATE TABLE IF NOT EXISTS `_backup_20260918_defending_broken_isles_objectives` LIKE `conditions`;

INSERT IGNORE INTO `_backup_20260918_defending_broken_isles_objectives`
SELECT *
FROM `conditions`
WHERE `SourceTypeOrReferenceId` = 22
  AND `SourceGroup` = 1
  AND `SourceEntry` IN (182, 183)
  AND `SourceId` = 10
  AND `ConditionTypeOrReference` = 41
  AND `ConditionValue1` IN (108260, 108261);

UPDATE `conditions`
SET `ConditionValue2` = `ConditionValue1`,
    `ConditionValue1` = 42537,
    `Comment` = CASE `NegativeCondition`
        WHEN 0 THEN 'Smart - Only If Player Completed Required Objective'
        ELSE 'Smart - Only If Player Has Not Completed Required Objective'
    END
WHERE `SourceTypeOrReferenceId` = 22
  AND `SourceGroup` = 1
  AND `SourceEntry` IN (182, 183)
  AND `SourceId` = 10
  AND `ConditionTypeOrReference` = 41
  AND `ConditionValue1` IN (108260, 108261)
  AND `ConditionValue2` = 0;
