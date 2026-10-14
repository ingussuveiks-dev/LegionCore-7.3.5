UPDATE eventobject_template SET ScriptName='eventobject_stormheim_heart'
WHERE entry=613 AND ScriptName IN('SmartEventObject','eventobject_stormheim_heart')
AND radius=10 AND SpellID=0 AND WorldSafeLocID=0 AND Flags=0;
UPDATE creature_template SET AIName='',ScriptName='npc_stormheim_heart_mother'
WHERE entry=91570 AND AIName IN('','SmartAI') AND ScriptName IN('','npc_stormheim_heart_mother')
AND NOT EXISTS(SELECT 1 FROM smart_scripts WHERE entryorguid=-294116 AND source_type=0 AND comment NOT LIKE '% - Heart spawn294116');
-- Native Azariah objective is SEQUENCED | TRACKED_ON_MINIMAP (3).
UPDATE quest_objectives SET Flags=3 WHERE ID=277831 AND QuestID=38414 AND Type=0
AND ObjectID=91737 AND StorageIndex=1 AND Amount=1 AND Flags=2;
-- SmartScript prefers DB spawn GUID rows over entry rows. Copy the three
-- native dialogue/link actions only for this exact outdoor dragon; remove
-- only its known corruption. Preserve any pre-existing custom GUID list.
INSERT INTO smart_scripts(entryorguid,source_type,id,link,Difficulties,event_type,event_phase_mask,event_chance,event_flags,event_param1,event_param2,event_param3,event_param4,event_param5,action_type,action_param1,action_param2,action_param3,action_param4,action_param5,action_param6,target_type,target_param1,target_param2,target_param3,target_param4,target_x,target_y,target_z,target_o,comment)
SELECT -294116,s.source_type,s.id,s.link,s.Difficulties,s.event_type,s.event_phase_mask,s.event_chance,s.event_flags,s.event_param1,s.event_param2,s.event_param3,s.event_param4,s.event_param5,s.action_type,IF(s.id=2,182091,s.action_param1),s.action_param2,s.action_param3,s.action_param4,s.action_param5,s.action_param6,s.target_type,s.target_param1,s.target_param2,s.target_param3,s.target_param4,s.target_x,s.target_y,s.target_z,s.target_o,CONCAT(s.comment,' - Heart spawn294116')
FROM smart_scripts s WHERE s.entryorguid=91570 AND s.source_type=0 AND s.id IN(0,1,2)
AND NOT EXISTS(SELECT 1 FROM smart_scripts WHERE entryorguid=-294116 AND source_type=0)
AND EXISTS(SELECT 1 FROM creature_template WHERE entry=91570 AND ScriptName='npc_stormheim_heart_mother')
AND EXISTS(SELECT 1 FROM creature WHERE guid=294116 AND id=91570 AND map=1220 AND zoneId=7541)
AND (SELECT COUNT(*) FROM smart_scripts WHERE entryorguid=91570 AND source_type=0 AND
 ((id=0 AND link=2 AND event_type=38 AND event_param1=1 AND event_param2=1 AND action_type=1 AND action_param1=3 AND action_param2=6000 AND target_type=7)
 OR(id=1 AND link=0 AND event_type=52 AND event_param1=3 AND event_param2=91570 AND action_type=1 AND action_param1=2 AND action_param2=6000 AND target_type=7)
 OR(id=2 AND link=0 AND event_type=61 AND action_type=28 AND action_param1=0 AND target_type=1)))=3;
-- An empty dangling hook, not an observed self-completion action.
UPDATE quest_template SET StartScript=0 WHERE ID=38414 AND StartScript=38414
AND NOT EXISTS(SELECT 1 FROM quest_start_scripts WHERE id=38414);
