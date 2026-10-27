-- Requires the dual native Nexus pickup/scene/accepted-return implementation.
-- Retain106756 native dialogue/teleport/data/unaura sequence; move its premature
-- portal to the real completed211954 presentation, keeping native9s and pose.
START TRANSACTION;
SET @al_graph := (SELECT SHA2(GROUP_CONCAT(CONCAT_WS('|',entryorguid,source_type,id,link,Difficulties,event_type,event_phase_mask,event_chance,event_flags,event_param1,event_param2,event_param3,event_param4,event_param5,action_type,action_param1,action_param2,action_param3,action_param4,action_param5,action_param6,target_type,target_param1,target_param2,target_param3,target_param4,target_x,target_y,target_z,target_o,comment) ORDER BY entryorguid,source_type,id SEPARATOR '\\n'),256)
 FROM smart_scripts WHERE(entryorguid=106756 AND source_type=0) OR(entryorguid=249814 AND source_type=1));
SET @al_conditions := (SELECT SHA2(GROUP_CONCAT(CONCAT_WS('|',SourceTypeOrReferenceId,SourceGroup,SourceEntry,SourceId,ElseGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2,ConditionValue3,NegativeCondition,ErrorTextId,ScriptName,Comment) ORDER BY SourceTypeOrReferenceId,SourceGroup,SourceEntry,SourceId,ElseGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2,ConditionValue3,NegativeCondition SEPARATOR '\\n'),256) FROM conditions WHERE (SourceTypeOrReferenceId=4 AND SourceGroup=249814) OR(SourceTypeOrReferenceId=17 AND SourceEntry=211954));
SET @al_bindings := (SELECT COUNT(*) FROM spell_script_names WHERE spell_id=211954 AND ScriptName IN('spell_lights_wrath_pickup_cast','spell_lights_wrath_pickup_scene'));
SET @al_old := @al_graph='fbc87c9434b46cc78db75736124c61674bd2ce50525f5c9c8feb4bd8c21ef260'
 AND (SELECT COUNT(*) FROM smart_scripts WHERE(entryorguid=106756 AND source_type=0) OR(entryorguid=249814 AND source_type=1))=8
 AND EXISTS(SELECT 1 FROM gameobject_template WHERE entry=249814 AND AIName='SmartGameObjectAI' AND ScriptName='go_artifact_acquisition')
 AND @al_bindings=0 AND NOT EXISTS(SELECT 1 FROM conditions WHERE (SourceTypeOrReferenceId=4 AND SourceGroup=249814) OR(SourceTypeOrReferenceId=17 AND SourceEntry=211954));
SET @al_new := @al_graph='1247b15914f72dfcb60c5e19e841c26521e239071c9e6375f91f139c37e75a38'
 AND (SELECT COUNT(*) FROM smart_scripts WHERE(entryorguid=106756 AND source_type=0) OR(entryorguid=249814 AND source_type=1))=6
 AND EXISTS(SELECT 1 FROM gameobject_template WHERE entry=249814 AND AIName='' AND ScriptName='go_lights_wrath_pickup')
 AND @al_bindings=2 AND @al_conditions='99cfb1ea1096c7416530f904261bd3312bc28ca72e5d70e6694f32d637fc87f0'
 AND (SELECT COUNT(*) FROM conditions WHERE (SourceTypeOrReferenceId=4 AND SourceGroup=249814) OR(SourceTypeOrReferenceId=17 AND SourceEntry=211954))=42;
SET @al_safe := (@al_old OR @al_new)
 AND EXISTS(SELECT 1 FROM gameobject_template WHERE entry=249814 AND type=3 AND Data0=1691 AND Data1=249814)
 AND EXISTS(SELECT 1 FROM gameobject_loot_template WHERE Entry=249814 AND Item=127857 AND Reference=0 AND Chance=100 AND QuestRequired=0 AND MinCount=1 AND MaxCount=1)
 AND (SELECT COUNT(*) FROM gameobject_loot_template WHERE Entry=249814)=1
 AND EXISTS(SELECT 1 FROM creature_template WHERE entry=106756 AND AIName='SmartAI' AND ScriptName='')
 AND EXISTS(SELECT 1 FROM creature WHERE guid=370546 AND id=106756 AND map=1583 AND PhaseId='6068')
 AND (SELECT SHA2(GROUP_CONCAT(CONCAT_WS('|',SourceTypeOrReferenceId,SourceGroup,SourceEntry,SourceId,ElseGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2,ConditionValue3,NegativeCondition,ErrorTextId,ScriptName,Comment) ORDER BY SourceGroup SEPARATOR '\\n'),256) FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceEntry=106756)='789ad91b47754f6bbf1afd8cea2a9d5d96ae11c583eaa738de2d4f106f1a45a2'
 AND (SELECT COUNT(*) FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceEntry=106756)=2
 AND NOT EXISTS(SELECT 1 FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceEntry=249814 AND SourceId=1)
 AND NOT EXISTS(SELECT 1 FROM smart_scripts WHERE entryorguid IN(106756,249814) AND source_type=9)
 AND NOT EXISTS(SELECT 1 FROM smart_scripts WHERE action_type=50 AND action_param1=248574 AND NOT(entryorguid=106756 AND source_type=0 AND id=4))
 AND NOT EXISTS(SELECT 1 FROM gameobject WHERE id IN(249814,248574))
 AND EXISTS(SELECT 1 FROM spell_scene WHERE MiscValue=1269 AND SceneScriptPackageID=1717 AND PlaybackFlags=21 AND CustomDuration=0 AND COALESCE(ScriptName,'')='')
 AND EXISTS(SELECT 1 FROM gameobject_template WHERE entry=248572 AND AIName='' AND ScriptName='go_lights_wrath_pickup')
 AND EXISTS(SELECT 1 FROM gameobject_template WHERE entry=248574 AND AIName='SmartGameObjectAI' AND ScriptName='go_lights_wrath_return')
 AND EXISTS(SELECT 1 FROM smart_scripts WHERE entryorguid=248574 AND source_type=1 AND id=0 AND SHA2(CONCAT_WS('|',entryorguid,source_type,id,link,Difficulties,event_type,event_phase_mask,event_chance,event_flags,event_param1,event_param2,event_param3,event_param4,event_param5,action_type,action_param1,action_param2,action_param3,action_param4,action_param5,action_param6,target_type,target_param1,target_param2,target_param3,target_param4,target_x,target_y,target_z,target_o,comment),256)='c3a236c1168fce863ab1332bbd645078f82fe430ec21fa31e728f32f139e8c0a')
 AND NOT EXISTS(SELECT 1 FROM spell_script_names WHERE ABS(spell_id)=211954 AND NOT(spell_id=211954 AND ScriptName IN('spell_lights_wrath_pickup_cast','spell_lights_wrath_pickup_scene')));
SET @al_guard := IF(@al_safe,'DO 0','SIGNAL SQLSTATE ''45000'' SET MESSAGE_TEXT=''Aluneth custom/missing native pickup or shared portal context; review before promotion''');
PREPARE al_check FROM @al_guard;
EXECUTE al_check;
DEALLOCATE PREPARE al_check;
UPDATE gameobject_template SET AIName='',ScriptName='go_lights_wrath_pickup' WHERE @al_safe AND entry=249814;
-- Map owns lifetime0 chest after its real summoner retires; preserve original pose.
UPDATE smart_scripts SET link=5,action_param3=4 WHERE @al_safe AND entryorguid=106756 AND source_type=0 AND id=3;
DELETE FROM smart_scripts WHERE @al_safe AND ((entryorguid=106756 AND source_type=0 AND id=4) OR(entryorguid=249814 AND source_type=1));
DELETE FROM conditions WHERE @al_safe AND ((SourceTypeOrReferenceId=4 AND SourceGroup=249814) OR(SourceTypeOrReferenceId=17 AND SourceEntry=211954));
INSERT INTO conditions(SourceTypeOrReferenceId,SourceGroup,SourceEntry,SourceId,ElseGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2,ConditionValue3,NegativeCondition,ErrorTextId,ScriptName,Comment)
SELECT 4,249814,127857,0,0,2,0,127857,1,1,1,0,'','Aluneth pickup - native final-step unowned loot' WHERE @al_safe
UNION ALL SELECT 4,249814,127857,0,0,8,0,42011,0,0,1,0,'','Aluneth pickup - native final-step unowned loot' WHERE @al_safe
UNION ALL SELECT 4,249814,127857,0,0,9,0,42011,0,0,0,0,'','Aluneth pickup - native final-step unowned loot' WHERE @al_safe
UNION ALL SELECT 4,249814,127857,0,0,15,0,128,0,0,0,0,'','Aluneth pickup - native final-step unowned loot' WHERE @al_safe
UNION ALL SELECT 4,249814,127857,0,0,22,0,1583,0,0,0,0,'','Aluneth pickup - native final-step unowned loot' WHERE @al_safe
UNION ALL SELECT 4,249814,127857,0,0,36,0,0,0,0,0,0,'','Aluneth pickup - native final-step unowned loot' WHERE @al_safe
UNION ALL SELECT 4,249814,127857,0,0,44,0,1101,6,0,0,0,'','Aluneth pickup - native final-step unowned loot' WHERE @al_safe
UNION ALL SELECT 4,249814,127857,0,1,2,0,127857,1,1,1,0,'','Aluneth pickup - native final-step unowned loot' WHERE @al_safe
UNION ALL SELECT 4,249814,127857,0,1,8,0,42011,0,0,1,0,'','Aluneth pickup - native final-step unowned loot' WHERE @al_safe
UNION ALL SELECT 4,249814,127857,0,1,15,0,128,0,0,0,0,'','Aluneth pickup - native final-step unowned loot' WHERE @al_safe
UNION ALL SELECT 4,249814,127857,0,1,22,0,1583,0,0,0,0,'','Aluneth pickup - native final-step unowned loot' WHERE @al_safe
UNION ALL SELECT 4,249814,127857,0,1,28,0,42011,0,0,0,0,'','Aluneth pickup - native final-step unowned loot' WHERE @al_safe
UNION ALL SELECT 4,249814,127857,0,1,36,0,0,0,0,0,0,'','Aluneth pickup - native final-step unowned loot' WHERE @al_safe
UNION ALL SELECT 4,249814,127857,0,1,44,0,1101,6,0,0,0,'','Aluneth pickup - native final-step unowned loot' WHERE @al_safe
UNION ALL SELECT 17,0,211954,0,0,2,0,127857,1,1,0,0,'','Aluneth pickup - actual owned native final-step cast' WHERE @al_safe
UNION ALL SELECT 17,0,211954,0,0,8,0,42011,0,0,1,0,'','Aluneth pickup - actual owned native final-step cast' WHERE @al_safe
UNION ALL SELECT 17,0,211954,0,0,9,0,42011,0,0,0,0,'','Aluneth pickup - actual owned native final-step cast' WHERE @al_safe
UNION ALL SELECT 17,0,211954,0,0,15,0,128,0,0,0,0,'','Aluneth pickup - actual owned native final-step cast' WHERE @al_safe
UNION ALL SELECT 17,0,211954,0,0,22,0,1583,0,0,0,0,'','Aluneth pickup - actual owned native final-step cast' WHERE @al_safe
UNION ALL SELECT 17,0,211954,0,0,36,0,0,0,0,0,0,'','Aluneth pickup - actual owned native final-step cast' WHERE @al_safe
UNION ALL SELECT 17,0,211954,0,0,44,0,1101,6,0,0,0,'','Aluneth pickup - actual owned native final-step cast' WHERE @al_safe
UNION ALL SELECT 17,0,211954,0,1,2,0,127857,1,1,0,0,'','Aluneth pickup - actual owned native final-step cast' WHERE @al_safe
UNION ALL SELECT 17,0,211954,0,1,8,0,42011,0,0,1,0,'','Aluneth pickup - actual owned native final-step cast' WHERE @al_safe
UNION ALL SELECT 17,0,211954,0,1,15,0,128,0,0,0,0,'','Aluneth pickup - actual owned native final-step cast' WHERE @al_safe
UNION ALL SELECT 17,0,211954,0,1,22,0,1583,0,0,0,0,'','Aluneth pickup - actual owned native final-step cast' WHERE @al_safe
UNION ALL SELECT 17,0,211954,0,1,28,0,42011,0,0,0,0,'','Aluneth pickup - actual owned native final-step cast' WHERE @al_safe
UNION ALL SELECT 17,0,211954,0,1,36,0,0,0,0,0,0,'','Aluneth pickup - actual owned native final-step cast' WHERE @al_safe
UNION ALL SELECT 17,0,211954,0,1,44,0,1101,6,0,0,0,'','Aluneth pickup - actual owned native final-step cast' WHERE @al_safe
UNION ALL SELECT 17,0,211954,0,2,2,0,127857,1,1,0,0,'','Aluneth pickup - actual owned native final-step cast' WHERE @al_safe
UNION ALL SELECT 17,0,211954,0,2,8,0,42011,0,0,1,0,'','Aluneth pickup - actual owned native final-step cast' WHERE @al_safe
UNION ALL SELECT 17,0,211954,0,2,9,0,42011,0,0,0,0,'','Aluneth pickup - actual owned native final-step cast' WHERE @al_safe
UNION ALL SELECT 17,0,211954,0,2,15,0,128,0,0,0,0,'','Aluneth pickup - actual owned native final-step cast' WHERE @al_safe
UNION ALL SELECT 17,0,211954,0,2,22,0,1583,0,0,0,0,'','Aluneth pickup - actual owned native final-step cast' WHERE @al_safe
UNION ALL SELECT 17,0,211954,0,2,36,0,0,0,0,0,0,'','Aluneth pickup - actual owned native final-step cast' WHERE @al_safe
UNION ALL SELECT 17,0,211954,0,2,44,0,1101,7,0,0,0,'','Aluneth pickup - actual owned native final-step cast' WHERE @al_safe
UNION ALL SELECT 17,0,211954,0,3,2,0,127857,1,1,0,0,'','Aluneth pickup - actual owned native final-step cast' WHERE @al_safe
UNION ALL SELECT 17,0,211954,0,3,8,0,42011,0,0,1,0,'','Aluneth pickup - actual owned native final-step cast' WHERE @al_safe
UNION ALL SELECT 17,0,211954,0,3,15,0,128,0,0,0,0,'','Aluneth pickup - actual owned native final-step cast' WHERE @al_safe
UNION ALL SELECT 17,0,211954,0,3,22,0,1583,0,0,0,0,'','Aluneth pickup - actual owned native final-step cast' WHERE @al_safe
UNION ALL SELECT 17,0,211954,0,3,28,0,42011,0,0,0,0,'','Aluneth pickup - actual owned native final-step cast' WHERE @al_safe
UNION ALL SELECT 17,0,211954,0,3,36,0,0,0,0,0,0,'','Aluneth pickup - actual owned native final-step cast' WHERE @al_safe
UNION ALL SELECT 17,0,211954,0,3,44,0,1101,7,0,0,0,'','Aluneth pickup - actual owned native final-step cast' WHERE @al_safe;
INSERT IGNORE INTO spell_script_names(spell_id,ScriptName)
SELECT 211954,'spell_lights_wrath_pickup_cast' WHERE @al_safe
UNION ALL SELECT 211954,'spell_lights_wrath_pickup_scene' WHERE @al_safe;
COMMIT;
