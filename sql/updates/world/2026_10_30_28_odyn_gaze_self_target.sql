-- 215997 targets its caster. Spell::InitExplicitTargets removes a redundant
-- object target before source17 conditions; requiring target1 rejects every cast.
-- Keep the native Warrior caster gate, including ordinary Dalaran use.
START TRANSACTION;
SET @odyn_self_safe :=
 (SELECT COUNT(*) IN(1,2) AND COUNT(*)=SUM(
   SourceGroup=0 AND SourceId=0 AND ElseGroup=0 AND ConditionTypeOrReference=15
   AND ConditionTarget IN(0,1) AND ConditionValue1=1 AND ConditionValue2=0
   AND ConditionValue3=0 AND NegativeCondition=0 AND ErrorTextId=0
   AND ScriptName='' AND Comment='Spell only if player is warr')
  AND SUM(ConditionTarget=0)=1
  FROM conditions WHERE SourceTypeOrReferenceId=17 AND SourceEntry=215997);
SET @odyn_self_guard := IF(@odyn_self_safe,'DO 0',
 'SIGNAL SQLSTATE ''45000'' SET MESSAGE_TEXT=''Odyn gaze self-target conditions differ; review before applying''');
PREPARE odyn_self_check FROM @odyn_self_guard;
EXECUTE odyn_self_check;
DEALLOCATE PREPARE odyn_self_check;
DELETE FROM conditions WHERE SourceTypeOrReferenceId=17 AND SourceEntry=215997
 AND ConditionTarget=1 AND @odyn_self_safe;
COMMIT;
