-- Loot-state action205 sends49110 before the delayed206436 cast. If Execute
-- already supplied49109, the scenario has advanced9->10 before spell admission.
-- Retain every native living/Warrior/owned-item/active/unrewarded/map gate.
START TRANSACTION;
SET @stromkar_scene_count=(SELECT COUNT(*) FROM conditions WHERE SourceTypeOrReferenceId=17 AND SourceEntry=206436);
SET @stromkar_scene_before=(@stromkar_scene_count=14 AND NOT EXISTS(
 SELECT 1 FROM conditions WHERE SourceTypeOrReferenceId=17 AND SourceEntry=206436 AND ElseGroup NOT IN(0,1)));
SET @stromkar_scene_safe=(@stromkar_scene_count IN(14,28) AND NOT EXISTS(
 SELECT 1 FROM conditions WHERE SourceTypeOrReferenceId=17 AND SourceEntry=206436 AND NOT IFNULL((
  SourceGroup=0 AND SourceId=0 AND ElseGroup IN(0,1,2,3) AND ConditionTarget=0
  AND ErrorTextId=0 AND ScriptName=''
  AND Comment='Stromkar: living active Warrior final-stage owned progression and delayed cast'
  AND (
   (ConditionTypeOrReference=2 AND ConditionValue1=128910 AND ConditionValue2=1 AND ConditionValue3=0 AND NegativeCondition=0)
   OR (ConditionTypeOrReference=8 AND ConditionValue1=41105 AND ConditionValue2=0 AND ConditionValue3=0 AND NegativeCondition=1)
   OR (ConditionTypeOrReference=IF(MOD(ElseGroup,2)=0,9,28) AND ConditionValue1=41105 AND ConditionValue2=0 AND ConditionValue3=0 AND NegativeCondition=0)
   OR (ConditionTypeOrReference=15 AND ConditionValue1=1 AND ConditionValue2=0 AND ConditionValue3=0 AND NegativeCondition=0)
   OR (ConditionTypeOrReference=22 AND ConditionValue1=1539 AND ConditionValue2=0 AND ConditionValue3=0 AND NegativeCondition=0)
   OR (ConditionTypeOrReference=36 AND ConditionValue1=0 AND ConditionValue2=0 AND ConditionValue3=0 AND NegativeCondition=0)
   OR (ConditionTypeOrReference=44 AND ConditionValue1=1037 AND ConditionValue2=9+FLOOR(ElseGroup/2) AND ConditionValue3=0 AND NegativeCondition=0)
  )),0)));
-- Unique native keys and seven allowed predicates must fill each whole group.
SET @stromkar_scene_safe=(@stromkar_scene_safe AND
 (SELECT COUNT(DISTINCT ElseGroup)*7=COUNT(*) FROM conditions WHERE SourceTypeOrReferenceId=17 AND SourceEntry=206436)
 AND (@stromkar_scene_before OR @stromkar_scene_count=28));
SET @stromkar_scene_guard=IF(@stromkar_scene_safe,'DO 0',
 'SIGNAL SQLSTATE ''45000'' SET MESSAGE_TEXT=''Stromkar delayed-scene conditions differ; review before applying''');
PREPARE stromkar_scene_check FROM @stromkar_scene_guard;
EXECUTE stromkar_scene_check;
DEALLOCATE PREPARE stromkar_scene_check;
INSERT INTO conditions(SourceTypeOrReferenceId,SourceGroup,SourceEntry,SourceId,ElseGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2,ConditionValue3,NegativeCondition,ErrorTextId,ScriptName,Comment)
SELECT SourceTypeOrReferenceId,SourceGroup,SourceEntry,SourceId,ElseGroup+2,ConditionTypeOrReference,ConditionTarget,ConditionValue1,
 IF(ConditionTypeOrReference=44,10,ConditionValue2),ConditionValue3,NegativeCondition,ErrorTextId,ScriptName,Comment
FROM conditions WHERE SourceTypeOrReferenceId=17 AND SourceEntry=206436 AND ElseGroup IN(0,1)
 AND @stromkar_scene_safe AND @stromkar_scene_before;
COMMIT;
