-- Type 100 is not a valid ChatMsg.  The loader already falls back to SAY (12),
-- which is also what this Yarzill dialogue line requires.
UPDATE `creature_text`
SET `Type` = 12
WHERE `CreatureID` = 23141
  AND `GroupID` = 0
  AND `ID` = 0
  AND `Type` = 100
  AND `BroadcastTextID` = 21825;

-- Broadcast text 111351 is absent from the 7.3.5 client data.  SmartAI still
-- sends the row's own "HELP!" text, just as the loader's existing fallback did.
UPDATE `creature_text`
SET `BroadcastTextID` = 0
WHERE `CreatureID` = 49869
  AND `GroupID` = 0
  AND `ID` = 0
  AND `Text` = 'HELP!'
  AND `BroadcastTextID` = 111351;
