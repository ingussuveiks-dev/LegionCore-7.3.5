-- Code-coupled: both GO factories,208011 spell/aura and218172 scoped return
-- must be linked before publishing this migration to the live updater.
-- Keep shared Mage106756/248574 native SmartAI and all shared218172 users.
START TRANSACTION;
SET @lw_pick_graph := (SELECT SHA2(GROUP_CONCAT(CONCAT_WS('|',entryorguid,source_type,id,link,Difficulties,event_type,event_phase_mask,event_chance,event_flags,event_param1,event_param2,event_param3,event_param4,event_param5,action_type,action_param1,action_param2,action_param3,action_param4,action_param5,action_param6,target_type,target_param1,target_param2,target_param3,target_param4,target_x,target_y,target_z,target_o,comment) ORDER BY entryorguid,id SEPARATOR '\n'),256)
 FROM smart_scripts WHERE entryorguid IN(248572,248574) AND source_type=1);
SET @lw_pick_conditions := (SELECT SHA2(GROUP_CONCAT(CONCAT_WS('|',SourceTypeOrReferenceId,SourceGroup,SourceEntry,SourceId,ElseGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2,ConditionValue3,NegativeCondition,ErrorTextId,ScriptName,Comment) ORDER BY SourceEntry,SourceGroup,SourceId,ElseGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2,ConditionValue3,NegativeCondition SEPARATOR '\n'),256)
 FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceEntry IN(248572,248574));
SET @lw_pick_bindings := (SELECT COUNT(*) FROM spell_script_names WHERE
 (spell_id=208011 AND ScriptName IN('spell_lights_wrath_pickup_cast','spell_lights_wrath_pickup_scene'))
 OR(spell_id=218172 AND ScriptName='spell_lights_wrath_return'));
SET @lw_pick_old :=
 EXISTS(SELECT 1 FROM gameobject_template WHERE entry=248572 AND AIName='SmartGameObjectAI' AND ScriptName='go_artifact_acquisition')
 AND EXISTS(SELECT 1 FROM gameobject_template WHERE entry=248574 AND AIName='SmartGameObjectAI' AND ScriptName='')
 AND @lw_pick_bindings=0
 AND @lw_pick_graph='ea75bb6cfcee936e08ce91949efcf4b547e8218e45d5f7600a13d3a2e17f77f2'
 AND (SELECT COUNT(*) FROM smart_scripts WHERE entryorguid IN(248572,248574) AND source_type=1)=3
 AND @lw_pick_conditions='4393eac1d796ecaf9d556ca8c46976f33846581f8f5ca1d88e0ac36bb7c3a1b2'
 AND (SELECT COUNT(*) FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceEntry IN(248572,248574))=28;
SET @lw_pick_new :=
 EXISTS(SELECT 1 FROM gameobject_template WHERE entry=248572 AND AIName='' AND ScriptName='go_lights_wrath_pickup')
 AND EXISTS(SELECT 1 FROM gameobject_template WHERE entry=248574 AND AIName='SmartGameObjectAI' AND ScriptName='go_lights_wrath_return')
 AND @lw_pick_bindings=3
 AND @lw_pick_graph='c3a236c1168fce863ab1332bbd645078f82fe430ec21fa31e728f32f139e8c0a'
 AND (SELECT COUNT(*) FROM smart_scripts WHERE entryorguid IN(248572,248574) AND source_type=1)=1
 AND NOT EXISTS(SELECT 1 FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceEntry IN(248572,248574));
SET @lw_pick_safe := (@lw_pick_old OR @lw_pick_new)
 AND EXISTS(SELECT 1 FROM gameobject_template WHERE entry=248572 AND type=3 AND Data0=1691 AND Data1=248572)
 AND EXISTS(SELECT 1 FROM gameobject_template WHERE entry=248574 AND type=22 AND Data0=218172)
 AND NOT EXISTS(SELECT 1 FROM gameobject WHERE id IN(248572,248574))
 AND NOT EXISTS(SELECT 1 FROM smart_scripts WHERE entryorguid IN(248572,248574) AND source_type=9)
 AND EXISTS(SELECT 1 FROM smart_scripts WHERE entryorguid=106756 AND source_type=0 AND id=4
 AND SHA2(CONCAT_WS('|',entryorguid,source_type,id,link,Difficulties,event_type,event_phase_mask,event_chance,event_flags,event_param1,event_param2,event_param3,event_param4,event_param5,action_type,action_param1,action_param2,action_param3,action_param4,action_param5,action_param6,target_type,target_param1,target_param2,target_param3,target_param4,target_x,target_y,target_z,target_o,comment),256)='95fcb8292b185e1d4496ea4898dd6f1687148ecc5e9432408d8eeb32714881ac')
 AND NOT EXISTS(SELECT 1 FROM smart_scripts WHERE action_type=50 AND action_param1=248574 AND NOT
 ((entryorguid=106756 AND source_type=0 AND id=4) OR(entryorguid=248572 AND source_type=1 AND id=1)))
 AND EXISTS(SELECT 1 FROM spell_scene WHERE MiscValue=1244 AND SceneScriptPackageID=1600 AND PlaybackFlags=21 AND CustomDuration=0 AND COALESCE(ScriptName,'')='')
 AND EXISTS(SELECT 1 FROM spell_target_position WHERE id=218172 AND target_map=1220
 AND ABS(target_position_x+828.61)<0.01 AND ABS(target_position_y-4371.63)<0.01 AND ABS(target_position_z-738.64)<0.01)
 AND NOT EXISTS(SELECT 1 FROM spell_script_names WHERE ABS(spell_id) IN(208011,218172) AND NOT
 ((spell_id=208011 AND ScriptName IN('spell_lights_wrath_pickup_cast','spell_lights_wrath_pickup_scene'))
 OR(spell_id=218172 AND ScriptName='spell_lights_wrath_return')));
SET @lw_pick_guard := IF(@lw_pick_safe,'DO 0',
 'SIGNAL SQLSTATE ''45000'' SET MESSAGE_TEXT=''Lights Wrath pickup custom/missing native graph or shared context; review before promotion''');
PREPARE lw_pick_check FROM @lw_pick_guard;
EXECUTE lw_pick_check;
DEALLOCATE PREPARE lw_pick_check;
UPDATE gameobject_template SET AIName='',ScriptName='go_lights_wrath_pickup' WHERE @lw_pick_safe AND entry=248572;
UPDATE gameobject_template SET ScriptName='go_lights_wrath_return' WHERE @lw_pick_safe AND entry=248574;
DELETE FROM smart_scripts WHERE @lw_pick_safe AND entryorguid=248572 AND source_type=1;
DELETE FROM conditions WHERE @lw_pick_safe AND SourceTypeOrReferenceId=22 AND SourceEntry=248572 AND SourceId=1;
INSERT IGNORE INTO spell_script_names(spell_id,ScriptName)
SELECT 208011,'spell_lights_wrath_pickup_cast' WHERE @lw_pick_safe
UNION ALL SELECT 208011,'spell_lights_wrath_pickup_scene' WHERE @lw_pick_safe
UNION ALL SELECT 218172,'spell_lights_wrath_return' WHERE @lw_pick_safe;
COMMIT;
