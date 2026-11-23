-- Tending the Garden follows the rewarded Lawn of the Dead finale.
UPDATE `quest_template_addon`
SET `PrevQuestID` = 28748
WHERE `ID` = 28751 AND `PrevQuestID` = 0 AND `NextQuestID` = 0
  AND `ExclusiveGroup` = 0 AND `SpecialFlags` = 1;
