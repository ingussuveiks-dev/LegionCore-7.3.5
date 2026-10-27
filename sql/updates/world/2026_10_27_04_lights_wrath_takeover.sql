-- Code-coupled: all four registrations must be in the promoted executable.
-- Original full31-field graph is recorded in LightsWrathTakeoverOriginal.json.
-- Replace the unrelated timer/forced duplicate credit with real native aura
-- completion, original-recipient cast, accepted criterion and creation retry.
START TRANSACTION;
SET @lights_takeover_graph := (SELECT SHA2(GROUP_CONCAT(CONCAT_WS('|',
 entryorguid,source_type,id,link,Difficulties,event_type,event_phase_mask,event_chance,event_flags,
 event_param1,event_param2,event_param3,event_param4,event_param5,action_type,
 action_param1,action_param2,action_param3,action_param4,action_param5,action_param6,
 target_type,target_param1,target_param2,target_param3,target_param4,target_x,target_y,target_z,target_o,comment)
 ORDER BY source_type,id SEPARATOR '\n'),256) FROM smart_scripts WHERE entryorguid=104985 AND source_type IN(0,9));
SET @lights_takeover_safe :=
 ((EXISTS(SELECT 1 FROM creature_template WHERE entry=104985 AND AIName='SmartAI' AND ScriptName='')
 AND @lights_takeover_graph='5cfa07807232eebfbba89962f598700960996a4df1bfca0d8acdc330a70ac8aa'
 AND (SELECT COUNT(*) FROM smart_scripts WHERE entryorguid=104985 AND source_type IN(0,9))=7)
 OR (EXISTS(SELECT 1 FROM creature_template WHERE entry=104985 AND AIName='' AND ScriptName='npc_lights_wrath_takeover')
 AND NOT EXISTS(SELECT 1 FROM smart_scripts WHERE entryorguid=104985 AND source_type IN(0,9))
 AND (SELECT COUNT(*) FROM spell_script_names WHERE
 (spell_id=207949 AND ScriptName IN('spell_lights_wrath_control_request','spell_lights_wrath_control_channel'))
 OR(spell_id=207996 AND ScriptName='spell_lights_wrath_control_result'))=3))
 AND EXISTS(SELECT 1 FROM creature_template WHERE entry=104985 AND npcflag=16777216 AND unit_flags=768 AND faction=2102)
 AND (SELECT COUNT(*) FROM creature WHERE id=104985)=1
 AND EXISTS(SELECT 1 FROM creature WHERE guid=370524 AND id=104985 AND map=1583)
 AND (SELECT COUNT(*) FROM npc_spellclick_spells WHERE npc_entry=104985)=1
 AND EXISTS(SELECT 1 FROM npc_spellclick_spells WHERE npc_entry=104985 AND spell_id=207949 AND cast_flags=1 AND user_type=0 AND add_npc_flag=1)
 AND NOT EXISTS(SELECT 1 FROM npc_spellclick_spells WHERE spell_id IN(207949,207996) AND npc_entry<>104985)
 AND NOT EXISTS(SELECT 1 FROM smart_scripts WHERE entryorguid=-370524
 OR(action_type=80 AND action_param1=104985 AND entryorguid<>104985))
 AND NOT EXISTS(SELECT 1 FROM conditions WHERE (SourceTypeOrReferenceId=17 AND SourceEntry IN(207949,207996))
 OR(SourceTypeOrReferenceId=18 AND SourceGroup=104985)
 OR(SourceTypeOrReferenceId=22 AND SourceEntry IN(104985,-370524)))
 AND NOT EXISTS(SELECT 1 FROM spell_script_names WHERE ABS(spell_id) IN(207949,207996) AND NOT
 ((spell_id=207949 AND ScriptName IN('spell_lights_wrath_control_request','spell_lights_wrath_control_channel'))
 OR(spell_id=207996 AND ScriptName='spell_lights_wrath_control_result')));
SET @lights_takeover_guard := IF(@lights_takeover_safe,'DO 0',
 'SIGNAL SQLSTATE ''45000'' SET MESSAGE_TEXT=''Lights Wrath takeover custom/missing native context; review before promotion''');
PREPARE lights_takeover_check FROM @lights_takeover_guard;
EXECUTE lights_takeover_check;
DEALLOCATE PREPARE lights_takeover_check;
UPDATE creature_template SET AIName='',ScriptName='npc_lights_wrath_takeover' WHERE @lights_takeover_safe AND entry=104985;
DELETE FROM smart_scripts WHERE @lights_takeover_safe AND entryorguid=104985 AND source_type IN(0,9);
INSERT IGNORE INTO spell_script_names(spell_id,ScriptName)
SELECT 207949,'spell_lights_wrath_control_request' WHERE @lights_takeover_safe
UNION ALL SELECT 207949,'spell_lights_wrath_control_channel' WHERE @lights_takeover_safe
UNION ALL SELECT 207996,'spell_lights_wrath_control_result' WHERE @lights_takeover_safe;
COMMIT;
