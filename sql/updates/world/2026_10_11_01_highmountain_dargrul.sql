-- 39027: owner-scoped Oro escort, Dargrul encounter and actual grasp escape.
UPDATE creature_template SET AIName='',ScriptName='npc_dargrul_escort' WHERE entry=97552;
UPDATE creature_template SET AIName='',ScriptName='npc_dargrul_grasp' WHERE entry=97781;
DELETE FROM smart_scripts WHERE entryorguid=97552 AND source_type=0;
-- Recovery starts the native controller only inside the cave, near its path.
DELETE FROM smart_scripts WHERE entryorguid=97553 AND source_type=0 AND event_type=19 AND event_param1=39027 AND action_type=85 AND action_param1=205051;
DELETE FROM spell_script_names WHERE spell_id=193017 AND ScriptName='spell_dargrul_escape';
INSERT INTO spell_script_names (spell_id,ScriptName) VALUES (193017,'spell_dargrul_escape');
-- The previous radius event credited every nearby player immediately and used
-- shared, one-shot actors. The player script now starts personal actor timelines.
UPDATE eventobject_template SET ScriptName='' WHERE entry=604 AND ScriptName='SmartEventObject';
DELETE FROM smart_scripts WHERE entryorguid=604 AND source_type=13;
DELETE FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceEntry=604 AND SourceId IN(10,13);
-- SourceId 13 is EVENTOBJECT; 10 is SCENE and did not gate this event at all.
UPDATE conditions SET SourceId=13 WHERE SourceTypeOrReferenceId=22 AND SourceGroup=1 AND SourceEntry=543 AND SourceId=10 AND ConditionValue1=41764;
DELETE FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceGroup=2 AND SourceEntry=543 AND SourceId=13;
INSERT INTO conditions (SourceTypeOrReferenceId,SourceGroup,SourceEntry,SourceId,ElseGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2,ConditionValue3,NegativeCondition,ErrorTextId,ScriptName,Comment) VALUES
(22,2,543,13,0,9,0,39027,0,0,0,0,'','39027 exit: active quest'),
(22,2,543,13,0,41,0,39027,97871,1,0,0,'','39027 exit: followed Oro'),
(22,2,543,13,0,41,0,39027,94610,1,0,0,'','39027 exit: confronted Dargrul'),
(22,2,543,13,0,41,0,39027,97781,1,0,0,'','39027 exit: escaped actual grasp'),
(22,2,543,13,0,41,0,39027,93805,1,1,0,'','39027 exit: not already credited');
-- Native 7.3.5 hidden/optional presentation. Exit still requires both via the
-- conditions above. Keep deployed storage slots 0/1 to preserve saved progress:
-- the server sends these slots in its quest query response (no character swap).
UPDATE quest_objectives SET Flags=12 WHERE ID IN(280130,280971) AND QuestID=39027;
