-- Cry Thunder38624: retain the summit, board Vethir, fight real personal
-- Hrydshal targets and leave only after150 kills. Keep shared entry AI intact.
SET @stormheim_cry_thunder_native = EXISTS (
 SELECT 1 FROM quest_template WHERE ID=38624 AND StartScript IN(0,38624))
 AND NOT EXISTS (SELECT 1 FROM quest_start_scripts WHERE id=38624 AND NOT (
  (delay=1 AND command=8 AND datalong=91767) OR
  (delay=5 AND command=7 AND datalong=38624) OR
  (delay=10 AND command=8 AND datalong=91768)))
 AND NOT EXISTS (SELECT 1 FROM smart_scripts WHERE source_type=0 AND entryorguid=-146706157 AND comment NOT LIKE '%Cry Thunder38624%')
 AND EXISTS (SELECT 1 FROM creature_template WHERE entry=92302 AND ScriptName='npc_stormheim_dragons' AND VehicleId=4133
  AND spell1=0 AND spell2=0 AND spell3=0 AND spell4=0)
 AND EXISTS (SELECT 1 FROM creature WHERE guid=388111 AND id=91767 AND map=1220 AND zoneId=7541)
 AND EXISTS (SELECT 1 FROM npc_spellclick_spells WHERE npc_entry=91767 AND spell_id=182921 AND cast_flags=1)
 AND (SELECT COUNT(*) FROM smart_scripts WHERE entryorguid=92218 AND source_type=0)=3
 AND EXISTS (SELECT 1 FROM smart_scripts WHERE entryorguid=92218 AND source_type=0 AND id=1 AND event_type=19 AND event_param1=38624 AND action_type=62 AND action_param1=1220 AND ABS(target_x-3070.21)<0.01 AND ABS(target_y-2249.24)<0.01 AND ABS(target_z-223.3)<0.01)
 AND NOT EXISTS (SELECT 1 FROM creature_template WHERE entry IN(92312,92367,92384,92342) AND (AIName NOT IN('','SmartAI') OR ScriptName NOT IN('','npc_stormheim_cry_thunder_target','npc_stormheim_cry_thunder_tempest')));

-- GUID-before-entry SmartAI list: omit only this actor's acceptance teleport.
INSERT INTO smart_scripts
SELECT -146706157,source_type,id,link,Difficulties,event_type,event_phase_mask,event_chance,event_flags,event_param1,event_param2,event_param3,event_param4,event_param5,action_type,action_param1,action_param2,action_param3,action_param4,action_param5,action_param6,target_type,target_param1,target_param2,target_param3,target_param4,target_x,target_y,target_z,target_o,CONCAT(comment,' - Cry Thunder38624')
FROM smart_scripts WHERE @stormheim_cry_thunder_native AND entryorguid=92218 AND source_type=0 AND id IN(0,3)
AND NOT EXISTS (SELECT 1 FROM (SELECT entryorguid FROM smart_scripts WHERE entryorguid=-146706157 AND source_type=0) AS existing);

UPDATE quest_template SET StartScript=0 WHERE @stormheim_cry_thunder_native AND ID=38624 AND StartScript=38624;
DELETE FROM quest_start_scripts WHERE @stormheim_cry_thunder_native AND id=38624;
UPDATE quest_template_addon SET SpecialFlags=SpecialFlags & ~2 WHERE @stormheim_cry_thunder_native AND ID=38624;

UPDATE creature_template SET AIName='',ScriptName='npc_stormheim_cry_thunder_target'
WHERE @stormheim_cry_thunder_native AND entry IN(92312,92367,92384);
UPDATE creature_template SET AIName='',ScriptName='npc_stormheim_cry_thunder_tempest'
WHERE @stormheim_cry_thunder_native AND entry=92342;

INSERT INTO spell_script_names(spell_id,ScriptName)
SELECT 182921,'spell_stormheim_cry_thunder_board' WHERE @stormheim_cry_thunder_native AND NOT EXISTS (SELECT 1 FROM spell_script_names WHERE spell_id=182921 AND ScriptName='spell_stormheim_cry_thunder_board');
INSERT INTO spell_script_names(spell_id,ScriptName)
SELECT 183058,'spell_stormheim_cry_thunder_attack' WHERE @stormheim_cry_thunder_native AND NOT EXISTS (SELECT 1 FROM spell_script_names WHERE spell_id=183058 AND ScriptName='spell_stormheim_cry_thunder_attack');
INSERT INTO spell_script_names(spell_id,ScriptName)
SELECT 183065,'spell_stormheim_cry_thunder_attack' WHERE @stormheim_cry_thunder_native AND NOT EXISTS (SELECT 1 FROM spell_script_names WHERE spell_id=183065 AND ScriptName='spell_stormheim_cry_thunder_attack');
INSERT INTO spell_script_names(spell_id,ScriptName)
SELECT 183042,'spell_stormheim_cry_thunder_attack' WHERE @stormheim_cry_thunder_native AND NOT EXISTS (SELECT 1 FROM spell_script_names WHERE spell_id=183042 AND ScriptName='spell_stormheim_cry_thunder_attack');
INSERT INTO spell_script_names(spell_id,ScriptName)
SELECT 183471,'spell_stormheim_cry_thunder_end' WHERE @stormheim_cry_thunder_native AND NOT EXISTS (SELECT 1 FROM spell_script_names WHERE spell_id=183471 AND ScriptName='spell_stormheim_cry_thunder_end');
INSERT INTO spell_script_names(spell_id,ScriptName)
SELECT 183072,'spell_stormheim_cry_thunder_tempest' WHERE @stormheim_cry_thunder_native AND NOT EXISTS (SELECT 1 FROM spell_script_names WHERE spell_id=183072 AND ScriptName='spell_stormheim_cry_thunder_tempest');
