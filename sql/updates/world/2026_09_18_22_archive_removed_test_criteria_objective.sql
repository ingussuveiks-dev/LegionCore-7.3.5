-- SHWAYDER TEST (34360) is an inaccessible internal test quest. Its old
-- build-22522 objective refers to CriteriaTree 53086, which is absent from the
-- final 7.3.5 client. Preserve the row, then remove only that stale objective;
-- the later build-23877 talk objective and the quest itself remain untouched.
CREATE TABLE IF NOT EXISTS `_backup_20260918_removed_test_criteria_objective`
LIKE `quest_objectives`;

REPLACE INTO `_backup_20260918_removed_test_criteria_objective`
SELECT *
FROM `quest_objectives`
WHERE `ID` = 286350
  AND `QuestID` = 34360
  AND `Type` = 14
  AND `ObjectID` = 53086
  AND `VerifiedBuild` = 22522;

DELETE FROM `quest_objectives`
WHERE `ID` = 286350
  AND `QuestID` = 34360
  AND `Type` = 14
  AND `ObjectID` = 53086
  AND `VerifiedBuild` = 22522;
