-- 7.3.5 Karazhan travel and the independent Deadwind Pass continuation.
-- 44556 keeps its historical predecessor 44944; do not invent an attunement.
-- Native portal 266290 uses 230118 and its existing map-0 destination.
INSERT INTO gameobject (id,map,position_x,position_y,position_z,orientation,rotation2,rotation3,spawntimesecs,animprogress,state)
SELECT 266290,1220,-851.462,4635.85,749.63,0,0,1,120,255,1
WHERE NOT EXISTS (SELECT 1 FROM gameobject WHERE id=266290 AND map=1220 AND ABS(position_x+851.462)<1 AND ABS(position_y-4635.85)<1);
INSERT INTO conditions (SourceTypeOrReferenceId,SourceEntry,ConditionTypeOrReference,ConditionValue1,ConditionValue2,Comment)
SELECT 17,230118,27,110,3,'Karazhan 7.3.5 portal requires level 110'
WHERE NOT EXISTS (SELECT 1 FROM conditions WHERE SourceTypeOrReferenceId=17 AND SourceEntry=230118 AND ConditionTypeOrReference=27 AND ConditionValue1=110 AND ConditionValue2=3);

-- Restore native clickable actors at existing cellar spawn anchors. These are
-- recovery placements within the original quest POIs, not claimed sniffed retail
-- actor coordinates. Four Eredar Portal-Keepers already stand at these sites.
INSERT INTO creature (id,map,position_x,position_y,position_z,orientation,spawntimesecs)
SELECT 115414,0,c.position_x+2,c.position_y,c.position_z,c.orientation,60 FROM creature c
WHERE c.guid IN(373187,373190,373203,373272) AND c.id=114314 AND c.map=0
AND NOT EXISTS (SELECT 1 FROM creature p WHERE p.id=115414 AND p.map=0 AND ABS(p.position_x-c.position_x-2)<1 AND ABS(p.position_y-c.position_y)<1);
INSERT INTO creature (id,map,position_x,position_y,position_z,orientation,spawntimesecs)
SELECT 115027,0,-11192,-2194.06,20.2818,0,60
WHERE NOT EXISTS (SELECT 1 FROM creature WHERE id=115027 AND map=0);
INSERT INTO creature (id,map,position_x,position_y,position_z,orientation,spawntimesecs)
SELECT 115037,0,-11178,-1934.38,-15.294,0,60
WHERE NOT EXISTS (SELECT 1 FROM creature WHERE id=115037 AND map=0);
INSERT INTO creature (id,map,position_x,position_y,position_z,orientation,spawntimesecs)
SELECT 114641,0,-11102.1,-2213.41,13.7372,0,60
WHERE NOT EXISTS (SELECT 1 FROM creature WHERE id=114641 AND map=0);

-- Existing npc_spellclick_spells already contain the native 229466 / 228208 /
-- 231458 spells. Their effect 1 awards the original objective credit.
INSERT INTO spell_script_names (spell_id,ScriptName)
SELECT 228271,'spell_karazhan_collect_sample' WHERE NOT EXISTS
(SELECT 1 FROM spell_script_names WHERE spell_id=228271 AND ScriptName='spell_karazhan_collect_sample');
INSERT INTO spell_script_names (spell_id,ScriptName)
SELECT 229466,'spell_karazhan_disable_node' WHERE NOT EXISTS
(SELECT 1 FROM spell_script_names WHERE spell_id=229466 AND ScriptName='spell_karazhan_disable_node');
INSERT INTO spell_script_names (spell_id,ScriptName)
SELECT 228208,'spell_karazhan_disable_node' WHERE NOT EXISTS
(SELECT 1 FROM spell_script_names WHERE spell_id=228208 AND ScriptName='spell_karazhan_disable_node');
INSERT INTO spell_script_names (spell_id,ScriptName)
SELECT 231458,'spell_karazhan_disable_node' WHERE NOT EXISTS
(SELECT 1 FROM spell_script_names WHERE spell_id=231458 AND ScriptName='spell_karazhan_disable_node');

-- Reset the interaction latch on respawn; preserve any independent custom AI.
UPDATE creature_template SET ScriptName='npc_karazhan_quest_node'
WHERE entry IN(115027,115037,115414) AND AIName='' AND ScriptName IN('','npc_karazhan_quest_node');
