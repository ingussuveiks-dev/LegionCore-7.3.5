-- Existing native SmartAI retry2/map ownership4/no-reset256; no new C++ binding.
-- Keep native intro once, restore its spawn phase on Reset, and never duplicate
-- an existing same-phase native chest or completed exit. No player credit.
START TRANSACTION;
SET @alc_graph := (SELECT SHA2(GROUP_CONCAT(CONCAT_WS('|',entryorguid,source_type,id,link,Difficulties,event_type,event_phase_mask,event_chance,event_flags,event_param1,event_param2,event_param3,event_param4,event_param5,action_type,action_param1,action_param2,action_param3,action_param4,action_param5,action_param6,target_type,target_param1,target_param2,target_param3,target_param4,target_x,target_y,target_z,target_o,comment) ORDER BY id SEPARATOR '\\n'),256) FROM smart_scripts WHERE entryorguid=106756 AND source_type=0);
SET @alc_conditions := (SELECT SHA2(GROUP_CONCAT(CONCAT_WS('|',SourceTypeOrReferenceId,SourceGroup,SourceEntry,SourceId,ElseGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2,ConditionValue3,NegativeCondition,ErrorTextId,ScriptName,Comment) ORDER BY SourceGroup,ElseGroup,ConditionTypeOrReference,ConditionValue1 SEPARATOR '\\n'),256) FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceEntry=106756);
SET @alc_old := @alc_graph='1247b15914f72dfcb60c5e19e841c26521e239071c9e6375f91f139c37e75a38' AND @alc_conditions='789ad91b47754f6bbf1afd8cea2a9d5d96ae11c583eaa738de2d4f106f1a45a2'
 AND (SELECT COUNT(*) FROM smart_scripts WHERE entryorguid=106756 AND source_type=0)=6
 AND (SELECT COUNT(*) FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceEntry=106756)=2;
SET @alc_new := @alc_graph='d7873fc8e5ac0cdaff10e58d85b88f03c262bb20404f1f9302fc4d75ec3fdce7' AND @alc_conditions='170beeb2b544c367c3a33cf8b70b92738c322f32f60cdcb48db754e7e2a2f65f'
 AND (SELECT COUNT(*) FROM smart_scripts WHERE entryorguid=106756 AND source_type=0)=8
 AND (SELECT COUNT(*) FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceEntry=106756)=8;
SET @alc_safe := (@alc_old OR @alc_new)
 AND EXISTS(SELECT 1 FROM creature_template WHERE entry=106756 AND AIName='SmartAI' AND ScriptName='')
 AND EXISTS(SELECT 1 FROM creature WHERE guid=370546 AND id=106756 AND map=1583 AND PhaseId='6068')
 AND (SELECT COUNT(*) FROM creature WHERE id=106756)=1
 AND NOT EXISTS(SELECT 1 FROM smart_scripts WHERE entryorguid=106756 AND source_type=9)
 AND EXISTS(SELECT 1 FROM gameobject_template WHERE entry=249814 AND AIName='' AND ScriptName='go_lights_wrath_pickup' AND type=3 AND Data0=1691 AND Data1=249814)
 AND EXISTS(SELECT 1 FROM gameobject_template WHERE entry=248574 AND AIName='SmartGameObjectAI' AND ScriptName='go_lights_wrath_return')
 AND NOT EXISTS(SELECT 1 FROM gameobject WHERE id IN(249814,248574))
 AND NOT EXISTS(SELECT 1 FROM smart_scripts WHERE action_type=50 AND action_param1=249814 AND NOT(entryorguid=106756 AND source_type=0 AND id IN(3,7)));
SET @alc_guard := IF(@alc_safe,'DO 0','SIGNAL SQLSTATE ''45000'' SET MESSAGE_TEXT=''Aluneth initial chest custom/missing native graph; review before promotion''');
PREPARE alc_check FROM @alc_guard;
EXECUTE alc_check;
DEALLOCATE PREPARE alc_check;
DELETE FROM smart_scripts WHERE @alc_safe AND entryorguid=106756 AND source_type=0;
INSERT INTO smart_scripts(entryorguid,source_type,id,link,Difficulties,event_type,event_phase_mask,event_chance,event_flags,event_param1,event_param2,event_param3,event_param4,event_param5,action_type,action_param1,action_param2,action_param3,action_param4,action_param5,action_param6,target_type,target_param1,target_param2,target_param3,target_param4,target_x,target_y,target_z,target_o,comment)
SELECT 106756,0,0,0,'',60,0,100,0,0,0,8000,8000,0,1,0,0,0,0,0,0,21,50,0,0,0,0,0,0,0,'Event Update - Say Text' WHERE @alc_safe
UNION ALL SELECT 106756,0,1,2,'',60,0,100,257,0,0,0,0,0,45,1,1,0,0,0,0,11,113142,60,0,0,0,0,0,0,'Event Update - Send Data' WHERE @alc_safe
UNION ALL SELECT 106756,0,2,3,'',61,0,100,1,0,0,0,0,0,62,1583,0,0,0,0,0,21,60,0,0,0,1122.48,1048.06,312.513,3.13018,'Link - Teleport Player' WHERE @alc_safe
UNION ALL SELECT 106756,0,3,5,'',61,0,100,1,0,0,0,0,0,22,1,0,0,0,0,0,1,0,0,0,0,0,0,0,0,'Link - Arm native retryable chest phase' WHERE @alc_safe
UNION ALL SELECT 106756,0,5,6,'',61,0,100,1,0,0,0,0,0,28,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,'Link - Unaura' WHERE @alc_safe
UNION ALL SELECT 106756,0,6,0,'',61,0,100,1,0,0,0,0,0,1,1,0,0,0,0,0,21,60,0,0,0,0,0,0,0,'Link - Say Text' WHERE @alc_safe
UNION ALL SELECT 106756,0,7,0,'',60,1,100,257,0,0,2000,2000,0,50,249814,0,6,0,0,0,8,0,0,0,0,1117.78,1048.03,314.619,2.09636,'Native final-stage chest - retry failed map-owned creation without replaying intro' WHERE @alc_safe
UNION ALL SELECT 106756,0,8,0,'',25,0,100,0,0,0,0,0,0,22,1,0,0,0,0,0,1,0,0,0,0,0,0,0,0,'Reset - restore pending native chest phase without resetting accepted intro or spawn' WHERE @alc_safe;
DELETE FROM conditions WHERE @alc_safe AND SourceTypeOrReferenceId=22 AND SourceEntry=106756;
INSERT INTO conditions(SourceTypeOrReferenceId,SourceGroup,SourceEntry,SourceId,ElseGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2,ConditionValue3,NegativeCondition,ErrorTextId,ScriptName,Comment)
SELECT 22,1,106756,0,0,44,1,1101,5,0,0,0,'','Smart Only On 6 Scenario Step' WHERE @alc_safe
UNION ALL SELECT 22,2,106756,0,0,30,1,248574,100,0,1,0,'','Aluneth initial chest - existing native chest or exit prevents replay and duplicates' WHERE @alc_safe
UNION ALL SELECT 22,2,106756,0,0,30,1,249814,100,0,1,0,'','Aluneth initial chest - existing native chest or exit prevents replay and duplicates' WHERE @alc_safe
UNION ALL SELECT 22,2,106756,0,0,44,1,1101,6,0,0,0,'','Smart Only On 7 Scenario Step' WHERE @alc_safe
UNION ALL SELECT 22,8,106756,0,0,30,1,248574,100,0,1,0,'','Aluneth initial chest - existing native chest or exit prevents replay and duplicates' WHERE @alc_safe
UNION ALL SELECT 22,8,106756,0,0,30,1,249814,100,0,1,0,'','Aluneth initial chest - existing native chest or exit prevents replay and duplicates' WHERE @alc_safe
UNION ALL SELECT 22,8,106756,0,0,44,1,1101,6,0,0,0,'','Aluneth initial chest - actual native final step on base actor' WHERE @alc_safe
UNION ALL SELECT 22,9,106756,0,0,44,1,1101,6,0,0,0,'','Aluneth initial chest - actual native final step on base actor' WHERE @alc_safe;
COMMIT;
