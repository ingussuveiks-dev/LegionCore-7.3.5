-- The previous contribution manager left all three Broken Shore buildings stuck in
-- UnderAttack and never advanced their timers. Start the repaired lifecycle cleanly.
UPDATE `worldstate_data`
SET `Value` = CASE
    WHEN `VariableID` IN (13226, 13301, 13305) THEN 1 -- Building
    WHEN `VariableID` IN (13211, 13303, 13306) THEN 1 -- First occurrence
    ELSE 0                                            -- Construction progress
END
WHERE `VariableID` IN (13226, 13436, 13211, 13301, 13437, 13303, 13305, 13438, 13306);
