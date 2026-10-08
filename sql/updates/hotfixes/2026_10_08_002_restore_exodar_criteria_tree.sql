-- CriteriaTree 7898 in client build 26972 checks Exodar reputation (Criteria
-- 5332, type 46, faction 930) under tree 7896, achievement 2761.
-- The old hotfix instead used quest criterion 11321 and nonexistent parent 5332.
-- Limit the repair to that exact broken mapping; preserve unrelated custom data.
START TRANSACTION;

UPDATE `criteria_tree`
SET `CriteriaID` = 5332, `Parent` = 7896, `OrderIndex` = 1, `VerifiedBuild` = 26972
WHERE `ID` = 7898 AND `CriteriaID` = 11321 AND `Parent` = 5332;
SET @exodar_tree_repaired := ROW_COUNT();

-- Remove the two old ambiguous notifications and invalidate the cached row.
SET @exodar_hotfix_id := (SELECT COALESCE(MAX(`Id`), 0) + 1 FROM `hotfix_data`);
DELETE FROM `hotfix_data`
WHERE `TableHash` = 1255424668 AND `RecordID` = 7898 AND @exodar_tree_repaired > 0;
INSERT INTO `hotfix_data` (`Id`, `TableHash`, `RecordID`, `Timestamp`, `Deleted`)
SELECT @exodar_hotfix_id, 1255424668, 7898, UNIX_TIMESTAMP(), 0
WHERE @exodar_tree_repaired > 0;

COMMIT;
