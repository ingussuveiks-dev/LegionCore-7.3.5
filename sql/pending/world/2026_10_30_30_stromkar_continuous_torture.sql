-- Requires the matching ritual-channel/torture AuraScripts and per-caster204125 fix.
-- Keep201032's native channel; its scoped controller replaces periodic missile201031
-- with the original Thoradin Shadow Torture204125/53383/62962/9141/chain4511.
START TRANSACTION;
SET @torture_bindings=(SELECT COUNT(*) FROM spell_script_names WHERE spell_id IN(201032,204125)
 OR ScriptName IN('spell_stromkar_ritual_channel','spell_stromkar_ritual_torture'));
SET @torture_targets=(SELECT COUNT(*) FROM conditions WHERE SourceTypeOrReferenceId=13 AND SourceEntry=204125);
SET @torture_safe=(
 @torture_bindings IN(0,2) AND NOT EXISTS(SELECT 1 FROM spell_script_names
 WHERE (spell_id IN(201032,204125) OR ScriptName IN('spell_stromkar_ritual_channel','spell_stromkar_ritual_torture'))
 AND NOT IFNULL(((spell_id=201032 AND ScriptName='spell_stromkar_ritual_channel')
 OR (spell_id=204125 AND ScriptName='spell_stromkar_ritual_torture')),0))
 AND @torture_targets IN(0,1) AND NOT EXISTS(SELECT 1 FROM conditions
 WHERE SourceTypeOrReferenceId=13 AND SourceEntry=204125 AND NOT IFNULL((
 SourceGroup=1 AND SourceId=0 AND ElseGroup=0 AND ConditionTypeOrReference=31 AND ConditionTarget=0
 AND ConditionValue1=3 AND ConditionValue2=103144 AND ConditionValue3=0 AND NegativeCondition=0
 AND ErrorTextId=0 AND ScriptName='' AND Comment='Stromkar: native Shadow Torture receiver Thoradin'),0))
 AND ((@torture_bindings=0 AND @torture_targets=0) OR (@torture_bindings=2 AND @torture_targets=1))
 AND (SELECT COUNT(*) FROM creature WHERE guid IN(370922,370936,370937,370943) AND map=1539 AND PhaseId='5739'
 AND id=IF(guid=370922,103144,101875))=4
 AND (SELECT COUNT(*) FROM creature_template WHERE entry IN(101875,103144) AND AIName='SmartAI' AND ScriptName='')=2
 AND (SELECT COUNT(*) FROM spell_script_names WHERE spell_id=201031 AND ScriptName='spell_stromkar_ritual_beam')=1
 AND (SELECT COUNT(*) FROM smart_scripts WHERE entryorguid=101875 AND source_type=0 AND id=6 AND link=0
 AND Difficulties='' AND event_type=1 AND event_phase_mask=0 AND event_chance=100 AND event_flags=0
 AND event_param1=1 AND event_param2=70 AND event_param3=1000 AND event_param4=1000 AND event_param5=0
 AND action_type=11 AND action_param1=201032 AND action_param2=0 AND action_param3=0
 AND action_param4=0 AND action_param5=0 AND action_param6=0 AND target_type=19 AND target_param1=103144
 AND target_param2=50 AND target_param3=0 AND target_param4=0 AND target_x=0 AND target_y=0 AND target_z=0 AND target_o=0)=1);
SET @torture_guard=IF(@torture_safe,'DO 0',
 'SIGNAL SQLSTATE ''45000'' SET MESSAGE_TEXT=''Stromkar continuous-torture inputs differ; review before applying''');
PREPARE torture_check FROM @torture_guard;
EXECUTE torture_check;
DEALLOCATE PREPARE torture_check;
INSERT INTO spell_script_names(spell_id,ScriptName)
SELECT 201032,'spell_stromkar_ritual_channel' WHERE @torture_safe AND @torture_bindings=0;
INSERT INTO spell_script_names(spell_id,ScriptName)
SELECT 204125,'spell_stromkar_ritual_torture' WHERE @torture_safe AND @torture_bindings=0;
INSERT INTO conditions(SourceTypeOrReferenceId,SourceGroup,SourceEntry,SourceId,ElseGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2,ConditionValue3,NegativeCondition,ErrorTextId,ScriptName,Comment)
SELECT 13,1,204125,0,0,31,0,3,103144,0,0,0,'','Stromkar: native Shadow Torture receiver Thoradin'
WHERE @torture_safe AND @torture_targets=0;
COMMIT;
