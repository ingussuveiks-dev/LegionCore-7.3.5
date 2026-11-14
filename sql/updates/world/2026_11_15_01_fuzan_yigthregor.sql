-- Code-coupled: expose only after matching Release AI/spell registrations.
-- Functional stationary airborne artillery reconstruction, not a recovered
-- retail waypoint. Preserve original pose/phase, native217786 attack schedule
-- and Korithis DATA1/1 death; never install a proximity-guessed taxi path.
START TRANSACTION;
SET @yigthregor_safe :=
 (SELECT COUNT(*) FROM creature WHERE guid=14670030 AND id=109820 AND map=1625 AND MovementType IN(0,2) AND PhaseId='6906' AND ABS(position_x-872.483)<0.001 AND ABS(position_y+2772.84)<0.001 AND ABS(position_z-211.671)<0.001)=1
 AND (SELECT COUNT(*) FROM creature_template WHERE entry=109820 AND AIName='SmartAI' AND ScriptName IN('','npc_fuzan_yigthregor'))=1
 AND NOT EXISTS(SELECT 1 FROM creature_addon WHERE guid=14670030)
 AND NOT EXISTS(SELECT 1 FROM creature_movement_override WHERE SpawnId=14670030 AND NOT(Ground<=>0 AND Flight<=>1 AND Swim IS NULL AND Rooted IS NULL AND Random IS NULL AND InteractionPauseTimer IS NULL))
 AND NOT EXISTS(SELECT 1 FROM spell_script_names WHERE spell_id=217787 AND ScriptName<>'spell_fuzan_felfire_bomb')
 AND (SELECT COUNT(*) FROM smart_scripts WHERE entryorguid=109820 AND source_type=0)=2
 AND EXISTS(SELECT 1 FROM smart_scripts WHERE entryorguid=109820 AND source_type=0 AND id=0 AND link=0 AND event_type=60 AND event_param1=5000 AND event_param2=6000 AND event_param3=6000 AND event_param4=10000 AND action_type=11 AND action_param1=217786 AND target_type=21 AND target_param1=90)
 AND EXISTS(SELECT 1 FROM smart_scripts WHERE entryorguid=109820 AND source_type=0 AND id=1 AND link=0 AND event_type=38 AND event_param1=1 AND event_param2=1 AND action_type=37 AND target_type=1);
UPDATE creature SET MovementType=0 WHERE guid=14670030 AND @yigthregor_safe;
UPDATE creature_template SET ScriptName='npc_fuzan_yigthregor' WHERE entry=109820 AND @yigthregor_safe;
INSERT INTO creature_movement_override(SpawnId,Ground,Flight)
 SELECT 14670030,0,1 WHERE @yigthregor_safe AND NOT EXISTS(SELECT 1 FROM creature_movement_override WHERE SpawnId=14670030);
INSERT INTO spell_script_names(spell_id,ScriptName)
 SELECT 217787,'spell_fuzan_felfire_bomb' WHERE @yigthregor_safe AND NOT EXISTS(SELECT 1 FROM spell_script_names WHERE spell_id=217787 AND ScriptName='spell_fuzan_felfire_bomb');
COMMIT;
