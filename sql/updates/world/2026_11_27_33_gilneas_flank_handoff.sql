-- Legion quest line 572 goes from Betrayal at Tempest's Reach to Flank the Forsaken.
START TRANSACTION;

SET @gilneas_flank_safe :=
    EXISTS (SELECT 1 FROM `creature_questender` WHERE `id`=37876 AND `quest`=24592)
    AND EXISTS (SELECT 1 FROM `creature_queststarter` WHERE `id`=37876 AND `quest`=24677)
    AND EXISTS (SELECT 1 FROM `quest_template` WHERE `ID`=24592 AND `RewardNextQuest` IN (0,24677))
    AND EXISTS (SELECT 1 FROM `quest_template` WHERE `ID`=24677 AND `RewardNextQuest`=24575)
    AND EXISTS (SELECT 1 FROM `quest_template_addon` WHERE `ID`=24592 AND `PrevQuestID`=24672 AND `NextQuestID` IN (0,24677))
    AND EXISTS (SELECT 1 FROM `quest_template_addon` WHERE `ID`=24677 AND `PrevQuestID` IN (0,24592) AND `NextQuestID`=24575);

SET @gilneas_flank_guard := IF(@gilneas_flank_safe, 'DO 0',
    'SIGNAL SQLSTATE ''45000'' SET MESSAGE_TEXT = ''Gilneas Flank the Forsaken: quest or actor context differs''');
PREPARE gilneas_flank_check FROM @gilneas_flank_guard;
EXECUTE gilneas_flank_check;
DEALLOCATE PREPARE gilneas_flank_check;

UPDATE `quest_template` SET `RewardNextQuest`=24677
WHERE @gilneas_flank_safe AND `ID`=24592 AND `RewardNextQuest`=0;

UPDATE `quest_template_addon` SET `NextQuestID`=24677
WHERE @gilneas_flank_safe AND `ID`=24592 AND `NextQuestID`=0;

UPDATE `quest_template_addon` SET `PrevQuestID`=24592
WHERE @gilneas_flank_safe AND `ID`=24677 AND `PrevQuestID`=0;

COMMIT;
