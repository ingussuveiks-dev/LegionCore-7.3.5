-- These seven one-time lessons must expose REWARDED to the academy's spell_area.
-- Keep NOT_REMOVE_SOURCE so students retain their training equipment.
UPDATE `quest_template_addon`
SET `SpecialFlags` = `SpecialFlags` & ~1
WHERE `ID` IN (30027,30033,30034,30035,30036,30037,30038)
  AND (`SpecialFlags` & 17) = 17;
