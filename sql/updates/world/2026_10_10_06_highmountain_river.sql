-- Whitewater Wash: native 7.3.5 kick/spray actions and parallel river quests.
UPDATE creature_template SET AIName='',ScriptName='npc_highmountain_whitewater_carp'
WHERE entry=95148 AND ScriptName IN('','npc_highmountain_whitewater_carp');
DELETE FROM smart_scripts WHERE (entryorguid=95148 AND source_type=0 AND event_type=8 AND event_param1=188447)
    OR (entryorguid=9514800 AND source_type=9);
DELETE FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceEntry=9514800 AND SourceId=9;

-- Native 190421 / SummonProperties 3359 creates player-allied grubs. Pass the
-- sprayed drogbar as the summon target, then attack that target (not its owner).
UPDATE smart_scripts SET action_type=85,action_param2=2,target_type=1,
    comment='Sprayer summons allied grubs at the sprayed drogbar'
WHERE source_type=0 AND entryorguid IN(95013,96124) AND event_type=61
    AND action_type IN(85,11) AND action_param1=190421;
UPDATE smart_scripts SET target_type=129,comment='Summoned grub attacks its spell target, not its summoner'
WHERE entryorguid=94688 AND source_type=0 AND id=0 AND event_type=54 AND action_type=49;
DELETE FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceId=0
    AND ((SourceEntry=95013 AND SourceGroup=1) OR (SourceEntry=96124 AND SourceGroup=2))
    AND ConditionTypeOrReference IN(9,36) AND ConditionTarget=0;
INSERT INTO conditions (SourceTypeOrReferenceId,SourceGroup,SourceEntry,SourceId,ElseGroup,
    ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2,ConditionValue3,
    NegativeCondition,ErrorTextId,ScriptName,Comment) VALUES
(22,1,95013,0,0,9,0,39277,0,0,0,0,'','Spray credit requires active Spray and Prey'),
(22,1,95013,0,0,36,0,0,0,0,0,0,'','Sprayer must be alive'),
(22,2,96124,0,0,9,0,39277,0,0,0,0,'','Spray credit requires active Spray and Prey'),
(22,2,96124,0,0,36,0,0,0,0,0,0,'','Sprayer must be alive');

-- All three are offered together after The Flow of the River. Native
-- QuestLine order is display order, not a forced serial dependency.
UPDATE quest_template_addon SET PrevQuestID=39496 WHERE ID IN(39277,39614);
UPDATE quest_template_addon SET NextQuestID=0 WHERE ID IN(39316,39614);
