-- Keep the existing native discovery/collection goals and require each real
-- hand-in. The player script restores the personal Bwu'ja actors on re-entry.
UPDATE `quest_template_addon` SET `PrevQuestID`=34309 WHERE `ID`=34314 AND `PrevQuestID`=0;
UPDATE `quest_template_addon` SET `PrevQuestID`=34315 WHERE `ID`=34316 AND `PrevQuestID`=0;
UPDATE `quest_template_addon` SET `PrevQuestID`=34316 WHERE `ID`=34381 AND `PrevQuestID`=0;
-- The personal shackle uses normal GO objective credit after verifying the key.
UPDATE `gameobject_template` SET `ScriptName`='go_bwuja_shackle'
WHERE `entry`=229414 AND `ScriptName` IN ('', 'go_bwuja_shackle');
