-- Turn the Keys38778: restore native personal summon destinations. The three
-- optional native markers persist which distinct stone was actually turned.
SET @stormheim_runewood_native =
 (SELECT COUNT(*) FROM creature_template WHERE entry IN(93182,93342,93343) AND AIName IN('','SmartAI') AND ScriptName IN('','npc_stormheim_runewood_stone'))=3
 AND (SELECT COUNT(*) FROM smart_scripts WHERE source_type=0 AND entryorguid IN(93182,93342,93343))=3
 AND (SELECT COUNT(*) FROM smart_scripts WHERE source_type=0 AND entryorguid IN(93182,93342,93343) AND id=0 AND event_type=8 AND event_param1=184579 AND action_type=80 AND action_param1=entryorguid*100)=3
 AND (SELECT COUNT(*) FROM npc_spellclick_spells WHERE npc_entry IN(93182,93342,93343) AND spell_id=184579 AND cast_flags=1)=3
 AND NOT EXISTS (SELECT 1 FROM spell_target_position WHERE ID IN(184582,184792,184794) AND NOT (
 (ID=184582 AND target_map=1220 AND ABS(target_position_x-3116.57)<0.01 AND ABS(target_position_y-1247.91)<0.01 AND ABS(target_position_z-125.037)<0.01) OR
 (ID=184792 AND target_map=1220 AND ABS(target_position_x-2992.34)<0.01 AND ABS(target_position_y-1126.41)<0.01 AND ABS(target_position_z-130.259)<0.01) OR
 (ID=184794 AND target_map=1220 AND ABS(target_position_x-3181.52)<0.01 AND ABS(target_position_y-1054.34)<0.01 AND ABS(target_position_z-117.037)<0.01)))
 AND EXISTS (SELECT 1 FROM quest_objectives WHERE QuestID=38778 AND ObjectID=93196 AND Type=0 AND Amount=3 AND StorageIndex=0)
 AND (SELECT COUNT(*) FROM quest_objectives WHERE QuestID=38778 AND ObjectID IN(93185,93340,93341) AND Amount=1 AND (Flags & 4)<>0)=3;

UPDATE creature_template SET AIName='',ScriptName='npc_stormheim_runewood_stone'
WHERE @stormheim_runewood_native AND entry IN(93182,93342,93343);

INSERT INTO spell_target_position(ID,target_map,target_position_x,target_position_y,target_position_z,target_orientation)
SELECT 184582,1220,3116.57,1247.91,125.037,0 WHERE @stormheim_runewood_native AND NOT EXISTS (SELECT 1 FROM spell_target_position WHERE ID=184582);
INSERT INTO spell_target_position(ID,target_map,target_position_x,target_position_y,target_position_z,target_orientation)
SELECT 184792,1220,2992.34,1126.41,130.259,0 WHERE @stormheim_runewood_native AND NOT EXISTS (SELECT 1 FROM spell_target_position WHERE ID=184792);
INSERT INTO spell_target_position(ID,target_map,target_position_x,target_position_y,target_position_z,target_orientation)
SELECT 184794,1220,3181.52,1054.34,117.037,0 WHERE @stormheim_runewood_native AND NOT EXISTS (SELECT 1 FROM spell_target_position WHERE ID=184794);

INSERT INTO spell_script_names(spell_id,ScriptName)
SELECT 184579,'spell_stormheim_runewood_turn' WHERE @stormheim_runewood_native AND NOT EXISTS (SELECT 1 FROM spell_script_names WHERE spell_id=184579 AND ScriptName='spell_stormheim_runewood_turn');
INSERT INTO spell_script_names(spell_id,ScriptName)
SELECT 184582,'spell_stormheim_runewood_summon' WHERE @stormheim_runewood_native AND NOT EXISTS (SELECT 1 FROM spell_script_names WHERE spell_id=184582 AND ScriptName='spell_stormheim_runewood_summon');
INSERT INTO spell_script_names(spell_id,ScriptName)
SELECT 184792,'spell_stormheim_runewood_summon' WHERE @stormheim_runewood_native AND NOT EXISTS (SELECT 1 FROM spell_script_names WHERE spell_id=184792 AND ScriptName='spell_stormheim_runewood_summon');
INSERT INTO spell_script_names(spell_id,ScriptName)
SELECT 184794,'spell_stormheim_runewood_summon' WHERE @stormheim_runewood_native AND NOT EXISTS (SELECT 1 FROM spell_script_names WHERE spell_id=184794 AND ScriptName='spell_stormheim_runewood_summon');
