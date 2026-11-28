-- The Hunt for Sylvanas 24902. Install alongside the corresponding scripts.
START TRANSACTION;

SET @hunt_source = (
    EXISTS (SELECT 1 FROM `quest_template` WHERE `ID`=24902 AND `StartScript`=0 AND `CompleteScript`=0 AND `RewardNextQuest`=24903)
    AND EXISTS (SELECT 1 FROM `quest_template_addon` WHERE `ID`=24902 AND `PrevQuestID`=24904 AND `NextQuestID`=24903 AND `SpecialFlags`=0)
    AND EXISTS (SELECT 1 FROM `creature` WHERE `guid`=149746 AND `map`=654 AND `id`=38611 AND `areaId`=4758)
    AND EXISTS (SELECT 1 FROM `creature_queststarter` WHERE `id`=38611 AND `quest`=24902)
    AND EXISTS (SELECT 1 FROM `creature_questender` WHERE `id`=38611 AND `quest`=24902)
    AND EXISTS (SELECT 1 FROM `creature_questender` WHERE `id`=38539 AND `quest`=24903)
    AND EXISTS (SELECT 1 FROM `creature_template` WHERE `entry`=38611 AND `AIName`='' AND `ScriptName`='' AND `npcflag`=3)
    AND NOT EXISTS (SELECT 1 FROM `quest_objectives` WHERE `QuestID`=24902 OR `ID`=940007)
    AND NOT EXISTS (SELECT 1 FROM `spell_script_names` WHERE `spell_id` IN (72470,72471,72239))
    AND NOT EXISTS (SELECT 1 FROM `creature_text` WHERE `CreatureID` IN (38507,38530,38533,38537,38611,38617) AND `GroupID` BETWEEN 9 AND 13)
);
SET @hunt_installed = (
    EXISTS (SELECT 1 FROM `creature_template` WHERE `entry`=38611 AND `ScriptName`='npc_gilneas_hunt_lorna')
    AND EXISTS (SELECT 1 FROM `quest_objectives` WHERE `ID`=940007 AND `QuestID`=24902 AND `Type`=0 AND `StorageIndex`=0 AND `ObjectID`=38530 AND `Amount`=1)
    AND (SELECT COUNT(*) FROM `quest_objectives` WHERE `QuestID`=24902)=1
    AND (SELECT COUNT(*) FROM `spell_script_names` WHERE (`spell_id`,`ScriptName`) IN
        ((72470,'spell_gilneas_hunt_tobias'),(72471,'spell_gilneas_hunt_call'),(72239,'spell_gilneas_hunt_scene')))=3
    AND (SELECT COUNT(*) FROM `creature_text` WHERE `comment`='The Hunt for Sylvanas - ordered dialogue')=15
);
SET @hunt_error = IF(@hunt_source OR @hunt_installed, 'DO 0',
    'SIGNAL SQLSTATE ''45000'' SET MESSAGE_TEXT = ''Gilneas hunt 24902 source differs from checked data''');
PREPARE gilneas_hunt_check FROM @hunt_error;
EXECUTE gilneas_hunt_check;
DEALLOCATE PREPARE gilneas_hunt_check;

-- The 26972 QuestObjective extract lacks this event. The historical route
-- identifies the living scene actor 38530 as a single completion credit.
INSERT INTO `quest_objectives`
    (`ID`,`QuestID`,`Type`,`StorageIndex`,`ObjectID`,`Amount`,`Flags`,`Flags2`,`TaskStep`,`Description`,`Bugged`,`VerifiedBuild`)
SELECT 940007,24902,0,0,38530,1,0,0,0,'Overhear the Banshee Queen',0,0 WHERE @hunt_source;

UPDATE `creature_template` SET `ScriptName`='npc_gilneas_hunt_lorna' WHERE `entry`=38611 AND @hunt_source;

INSERT INTO `spell_script_names` (`spell_id`,`ScriptName`)
SELECT 72470,'spell_gilneas_hunt_tobias' WHERE @hunt_source
UNION ALL SELECT 72471,'spell_gilneas_hunt_call' WHERE @hunt_source
UNION ALL SELECT 72239,'spell_gilneas_hunt_scene' WHERE @hunt_source;

INSERT INTO `creature_text`
    (`CreatureID`,`GroupID`,`ID`,`Text`,`Type`,`Language`,`Probability`,`Emote`,`Duration`,`Sound`,`BroadcastTextID`,`comment`)
SELECT dialogue.`Speaker`, dialogue.`GroupId`, 0, '', dialogue.`Type`, 0, 100, dialogue.`Emote`, 0, dialogue.`Sound`, dialogue.`Broadcast`,
    'The Hunt for Sylvanas - ordered dialogue'
FROM (
    SELECT 38611 `Speaker`, 9 `GroupId`, 42 `Type`, 0 `Emote`, 0 `Sound`, 38400 `Broadcast`
    UNION ALL SELECT 38507,10,12,15,0,38399
    UNION ALL SELECT 38507,11,12,0,0,38401
    UNION ALL SELECT 38507,12,12,0,0,38402
    UNION ALL SELECT 38507,13,12,0,0,38403
    UNION ALL SELECT 38617,10,14,1,0,38514
    UNION ALL SELECT 38530,10,12,1,20492,38422
    UNION ALL SELECT 38530,11,12,0,20493,38423
    UNION ALL SELECT 38530,12,12,0,20494,38424
    UNION ALL SELECT 38530,13,12,0,20495,38425
    UNION ALL SELECT 38533,10,12,1,22620,38426
    UNION ALL SELECT 38533,11,12,274,22621,38427
    UNION ALL SELECT 38533,12,12,66,22622,38428
    UNION ALL SELECT 38537,10,12,66,20917,38440
    UNION ALL SELECT 38537,11,12,66,20918,38441
) AS dialogue
WHERE @hunt_source;

COMMIT;
