-- Risking It All: Ji alternates with Aysa using the original three broadcast texts.
UPDATE `creature_text`
SET `GroupID` = CASE `BroadcastTextID`
    WHEN 60189 THEN 0
    WHEN 60191 THEN 1
    WHEN 60193 THEN 2
END
WHERE `CreatureID` = 60741 AND `GroupID` = 3 AND `ID` = 0
  AND `BroadcastTextID` IN (60189, 60191, 60193);
