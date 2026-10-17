-- 39092: four completed shrine interactions precede personal native Communing.
SET @communion_repair=(
 (SELECT COUNT(*) FROM creature_template WHERE entry=94393 AND gossip_menu_id=18543 AND npcflag=1 AND AIName='SmartAI' AND ScriptName IN('','npc_stormheim_ascendant_statue'))=1
 AND (SELECT COUNT(*) FROM quest_objectives WHERE QuestID=39092 AND Type=2 AND ObjectID IN(242994,242995,242996,242998) AND Amount=1)=4
 AND (SELECT COUNT(*) FROM quest_objectives WHERE QuestID=39092 AND Type=0 AND ObjectID=94477 AND StorageIndex=4 AND Flags=2 AND Amount=1)=1
 AND NOT EXISTS(SELECT 1 FROM smart_scripts WHERE entryorguid=94393 AND source_type=0 AND NOT(link=0 AND event_type=62 AND event_param1=18543 AND event_param2=0 AND target_type=7 AND ((id=0 AND action_type=33 AND action_param1=94477) OR (id=1 AND action_type=72 AND action_param1=0))))
 AND NOT EXISTS(SELECT 1 FROM conditions WHERE SourceTypeOrReferenceId=15 AND SourceGroup=18543 AND SourceEntry=0 AND Comment NOT LIKE 'Skold-Ashil communion%')
);
UPDATE creature_template SET ScriptName='npc_stormheim_ascendant_statue' WHERE @communion_repair AND entry=94393;
-- Both captured click rows move into the AI: personal cast and gossip close.
DELETE FROM smart_scripts WHERE @communion_repair AND entryorguid=94393 AND source_type=0 AND link=0 AND event_type=62 AND event_param1=18543 AND event_param2=0 AND target_type=7 AND ((id=0 AND action_type=33 AND action_param1=94477) OR (id=1 AND action_type=72 AND action_param1=0));
INSERT INTO spell_script_names(spell_id,ScriptName)
SELECT 186982,'spell_stormheim_communing' WHERE @communion_repair AND NOT EXISTS(SELECT 1 FROM spell_script_names WHERE spell_id=186982 AND ScriptName='spell_stormheim_communing');
INSERT INTO conditions(SourceTypeOrReferenceId,SourceGroup,SourceEntry,SourceId,ElseGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2,ConditionValue3,NegativeCondition,ErrorTextId,ScriptName,Comment)
SELECT 15,18543,0,0,0,9,0,39092,0,0,0,0,'','Skold-Ashil communion requires active quest'
WHERE @communion_repair AND NOT EXISTS(SELECT 1 FROM conditions WHERE SourceTypeOrReferenceId=15 AND SourceGroup=18543 AND SourceEntry=0 AND ConditionTypeOrReference=9 AND ConditionValue1=39092);
INSERT INTO conditions(SourceTypeOrReferenceId,SourceGroup,SourceEntry,SourceId,ElseGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2,ConditionValue3,NegativeCondition,ErrorTextId,ScriptName,Comment)
SELECT 15,18543,0,0,0,41,0,39092,s.entry,1,0,0,'','Skold-Ashil communion requires all four shrines'
FROM(SELECT 242994 entry UNION ALL SELECT 242995 UNION ALL SELECT 242996 UNION ALL SELECT 242998) s
WHERE @communion_repair AND NOT EXISTS(SELECT 1 FROM conditions c WHERE c.SourceTypeOrReferenceId=15 AND c.SourceGroup=18543 AND c.SourceEntry=0 AND c.ConditionTypeOrReference=41 AND c.ConditionValue1=39092 AND c.ConditionValue2=s.entry);
