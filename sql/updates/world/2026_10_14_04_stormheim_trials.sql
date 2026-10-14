-- Both Ancient Trials routes are native travel/hand-in quests, without a
-- synthetic kill marker or scripted flight. Muninn should play the same Havi
-- invitation as the existing Horde raven; progression remains independent.
INSERT INTO smart_scripts (entryorguid,source_type,id,link,event_type,event_phase_mask,event_chance,event_flags,event_param1,action_type,action_param1,target_type,comment)
SELECT 97306,0,0,0,19,0,100,0,38210,85,183614,7,'Muninn - The Ancient Trials accepted - Havi invitation'
WHERE NOT EXISTS(SELECT 1 FROM smart_scripts WHERE entryorguid=97306 AND source_type=0 AND id=0);

-- 7.3.5 replaced Stew Stirred with three separate ingredient objectives.
-- Remove only this known older definition sharing the veggies storage slot.
DELETE FROM quest_objectives WHERE ID=279982 AND QuestID=38331 AND Type=0
AND ObjectID=98244 AND StorageIndex=0 AND Amount=1 AND Flags=0 AND VerifiedBuild=21287;

UPDATE gameobject_template SET ScriptName='go_stormheim_havi_ingredient'
WHERE entry IN(252074,252076,252080) AND ScriptName IN('','go_stormheim_havi_ingredient');
UPDATE creature_template SET ScriptName='npc_stormheim_havi_actor'
WHERE entry IN(110604,110605,110608,110609) AND ScriptName IN('','npc_stormheim_havi_actor');
INSERT IGNORE INTO spell_script_names(spell_id,ScriptName) VALUES
(219924,'spell_stormheim_havi_impact'),(219936,'spell_stormheim_havi_impact'),(219947,'spell_stormheim_havi_impact');

-- Remove only the exact duplicate of the common unphased Havi quest giver.
DELETE duplicate FROM creature duplicate JOIN creature original ON original.guid=340423
WHERE duplicate.guid=341181 AND duplicate.id=96254 AND original.id=96254
AND duplicate.map=1220 AND original.map=1220 AND duplicate.PhaseId=original.PhaseId
AND duplicate.phaseMask=original.phaseMask AND duplicate.spawnMask=original.spawnMask
AND duplicate.npcflag=original.npcflag AND duplicate.unit_flags=original.unit_flags
AND ABS(duplicate.position_x-original.position_x)<0.01
AND ABS(duplicate.position_y-original.position_y)<0.01
AND ABS(duplicate.position_z-original.position_z)<0.01
AND ABS(duplicate.orientation-original.orientation)<0.01;
