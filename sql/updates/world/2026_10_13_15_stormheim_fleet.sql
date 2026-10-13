-- Complete both native map-1475 mission lifecycles, leaving shared actors'
-- other maps and existing custom scripts intact.
INSERT INTO instance_template(map,parent,script,allowMount,bonusChance)
 VALUES(1475,0,'instance_stormheim_fleet',0,0)
 ON DUPLICATE KEY UPDATE script=IF(script IN('','instance_stormheim_fleet'),'instance_stormheim_fleet',script);
UPDATE creature_template SET ScriptName='npc_stormheim_fleet_actor'
 WHERE entry IN(92930,92957,93113,93173,93433,93479,93486,93490,93496,93506,93560,93563,93603,93960,93961,93962,94318,94420,94517,94550,94576,94737,94809)
 AND ScriptName IN('','npc_stormheim_fleet_actor');
UPDATE gameobject_template SET ScriptName='go_stormheim_fleet_flare'
 WHERE entry IN(242380,243244) AND ScriptName IN('','go_stormheim_fleet_flare');
UPDATE gameobject_template SET ScriptName='go_stormheim_fleet_parachute'
 WHERE entry=240210 AND ScriptName IN('','go_stormheim_fleet_parachute');
DELETE FROM npc_spellclick_spells WHERE npc_entry IN(94517,94576) AND spell_id=43671 AND cast_flags=1 AND user_type=0;
INSERT INTO npc_spellclick_spells(npc_entry,spell_id,cast_flags,user_type,add_npc_flag)
 VALUES(94517,43671,1,0,0),(94576,43671,1,0,0);
-- The AI sets the flag only on an owned scenario ride; shared world copies do
-- not gain a spellclick flag from these rows.
DELETE FROM spell_script_names WHERE (spell_id=186010 AND ScriptName='spell_stormheim_fleet_flare') OR (spell_id=43671 AND ScriptName='spell_stormheim_fleet_board') OR
 (spell_id IN(185614,185616) AND ScriptName='spell_stormheim_fleet_finale');
INSERT INTO spell_script_names(spell_id,ScriptName) VALUES
 (43671,'spell_stormheim_fleet_board'),(185614,'spell_stormheim_fleet_finale'),(185616,'spell_stormheim_fleet_finale'),(186010,'spell_stormheim_fleet_flare');
INSERT INTO creature_template_spell(entry,spell,difficultyMask,Difficulties,comment)
 SELECT 92957,184420,0,'','@Flak Blast' WHERE NOT EXISTS(SELECT 1 FROM creature_template_spell WHERE entry=92957 AND spell=184420);
-- This scenario's Genn copy was the only combat actor with an inactive 0..0
-- range. Match the active Legion encounter range of Nathanos and the captains.
UPDATE creature_template_scaling SET LevelScalingMin=98,LevelScalingMax=110
 WHERE Entry=94318 AND LevelScalingMin=0 AND LevelScalingMax=0;
