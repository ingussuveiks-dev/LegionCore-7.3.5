-- The native kill-credit objective and the returned crystal complete 8490.
UPDATE `quest_template_addon`
SET `SpecialFlags` = `SpecialFlags` & ~2
WHERE `ID` = 8490 AND (`SpecialFlags` & 2) <> 0
    AND EXISTS (SELECT 1 FROM `quest_objectives` WHERE `QuestID` = 8490 AND `Type` = 0 AND `ObjectID` = 16364 AND `Amount` = 1)
    AND EXISTS (SELECT 1 FROM `quest_objectives` WHERE `QuestID` = 8490 AND `Type` = 1 AND `ObjectID` = 22693 AND `Amount` = 1);

-- Remove only the legacy duplicate area-event and failure actions.
DELETE FROM `smart_scripts`
WHERE (`entryorguid` = 1636400 AND `source_type` = 9 AND `id` = 7 AND `action_type` = 15 AND `action_param1` = 8490)
   OR (`entryorguid` = 16364 AND `source_type` = 0 AND `id` = 3 AND `action_type` = 6 AND `action_param1` = 8490);
