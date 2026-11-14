-- Code-coupled: expose to updater only after the matching Release is promoted.
-- User-approved functional departure reconstruction, not a recovered retail
-- spline. Native109441/4577/16264,217818/217820 and original ground return pad.
-- Preserve original109440 SmartAI rows; the derived gossip selection defers
-- native credit until the real player joins seat0.
START TRANSACTION;
SET @fuzan_return_safe :=
 (SELECT COUNT(*) FROM creature_template WHERE entry=109440 AND AIName='SmartAI' AND gossip_menu_id=19932 AND ScriptName IN('','npc_fuzan_return_guide'))=1
 AND (SELECT COUNT(*) FROM creature_template WHERE entry=109441 AND VehicleId=4577 AND AIName='' AND ScriptName IN('','npc_fuzan_return_vehicle'))=1
 AND NOT EXISTS(SELECT 1 FROM spell_script_names WHERE spell_id IN(217818,217820) AND NOT (spell_id=217818 AND ScriptName='spell_fuzan_return_boarding' OR spell_id=217820 AND ScriptName='spell_fuzan_return_transfer'))
 AND (SELECT COUNT(*) FROM smart_scripts WHERE entryorguid=109440 AND source_type=0)=3
 AND EXISTS(SELECT 1 FROM smart_scripts WHERE entryorguid=109440 AND source_type=0 AND id=0 AND link=1 AND event_type=62 AND event_param1=19932 AND event_param2=0 AND action_type=33 AND action_param1=109440 AND target_type=7)
 AND EXISTS(SELECT 1 FROM smart_scripts WHERE entryorguid=109440 AND source_type=0 AND id=1 AND link=2 AND event_type=61 AND action_type=205 AND action_param1=92 AND action_param2=52751 AND target_type=7)
 AND EXISTS(SELECT 1 FROM smart_scripts WHERE entryorguid=109440 AND source_type=0 AND id=2 AND link=0 AND event_type=61 AND action_type=62 AND action_param1=1514 AND target_type=7 AND ABS(target_x-901.691)<0.001 AND ABS(target_y-3605.27)<0.001 AND ABS(target_z-193.116)<0.001);
UPDATE creature_template SET ScriptName='npc_fuzan_return_guide' WHERE entry=109440 AND @fuzan_return_safe;
UPDATE creature_template SET ScriptName='npc_fuzan_return_vehicle' WHERE entry=109441 AND @fuzan_return_safe;
INSERT INTO spell_script_names(spell_id,ScriptName)
 SELECT 217818,'spell_fuzan_return_boarding' WHERE @fuzan_return_safe AND NOT EXISTS(SELECT 1 FROM spell_script_names WHERE spell_id=217818 AND ScriptName='spell_fuzan_return_boarding');
INSERT INTO spell_script_names(spell_id,ScriptName)
 SELECT 217820,'spell_fuzan_return_transfer' WHERE @fuzan_return_safe AND NOT EXISTS(SELECT 1 FROM spell_script_names WHERE spell_id=217820 AND ScriptName='spell_fuzan_return_transfer');
COMMIT;
