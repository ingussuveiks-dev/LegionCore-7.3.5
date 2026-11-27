-- Old Fillmaff offers Memories of Ebonchill after The Mage Hunter is turned in.
UPDATE `quest_template_addon`
SET `PrevQuestID` = 42479
WHERE `ID` = 42429 AND `PrevQuestID` = 0;
