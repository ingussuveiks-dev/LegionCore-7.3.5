START TRANSACTION;
-- Keep the existing three alternative Return to the Grove prerequisites.
-- Change only the premature credit in the scene-start chain; keep 40122's ride.
UPDATE smart_scripts SET action_type=72,action_param1=0,comment='The Emerald Queen - close gossip; native scene completion spell supplies credit'
WHERE entryorguid=91109 AND source_type=0 AND id=5 AND action_type=33 AND action_param1=92742;
INSERT INTO spell_script_names (spell_id,ScriptName)
SELECT 197680,'spell_valsharah_summon_ysera_ritual' FROM DUAL
WHERE NOT EXISTS(SELECT 1 FROM spell_script_names WHERE spell_id=197680 AND ScriptName='spell_valsharah_summon_ysera_ritual');
-- Preserve the two quest-accept summon actions. Personal AI uses the same path
-- but checks that the owner follows instead of crediting at NPC arrival alone.
DELETE FROM smart_scripts WHERE entryorguid=103022 AND source_type=0 AND id IN(0,1,2,5,6);
DELETE FROM smart_scripts WHERE entryorguid=10302200 AND source_type=9;
UPDATE creature_template SET AIName='',ScriptName='npc_valsharah_path_tyrande' WHERE entry=103022 AND ScriptName IN('','npc_valsharah_path_tyrande');
-- Keep the legitimate meeting/proximity objective (row 0) on the static NPC.
DELETE FROM smart_scripts WHERE entryorguid=104739 AND source_type=0 AND id IN(1,2,3,4,5);
DELETE FROM smart_scripts WHERE entryorguid IN(10473900,10473902,10578639) AND source_type=9;
DELETE FROM smart_scripts WHERE entryorguid IN(104643,104644,104646) AND source_type=0 AND ((event_type=25 AND action_type=49) OR (event_type=6 AND action_type=45));
UPDATE creature_template SET AIName='',ScriptName='npc_valsharah_vigil_tyrande' WHERE entry=104739 AND ScriptName IN('','npc_valsharah_vigil_tyrande');
UPDATE creature_template SET AIName='',ScriptName='npc_valsharah_vigil_enemy' WHERE entry IN(104643,104644,104646) AND ScriptName IN('','npc_valsharah_vigil_enemy');
COMMIT;
