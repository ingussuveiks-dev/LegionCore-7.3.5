-- Native209457 effect0 is nearby-entry38,not an explicit-unit target.
-- Keep its native vehicle/seat action;select living Grash in the real finale.
START TRANSACTION;
SET @sharasdal_carry_safe :=
 EXISTS(SELECT 1 FROM creature_template WHERE entry=105298 AND VehicleId=4720 AND ScriptName='npc_grash')
 AND EXISTS(SELECT 1 FROM creature_template WHERE entry=104856 AND ScriptName='npc_lady_zitrin')
 AND EXISTS(SELECT 1 FROM creature WHERE guid=362092 AND id=105298 AND map=1600)
 AND EXISTS(SELECT 1 FROM creature WHERE guid=362095 AND id=104856 AND map=1600)
 AND (SELECT COUNT(*) FROM spell_script_names WHERE spell_id=209457)=0
 AND ((SELECT COUNT(*) FROM conditions WHERE SourceTypeOrReferenceId=13 AND SourceEntry=209457)=0
  OR ((SELECT COUNT(*) FROM conditions WHERE SourceTypeOrReferenceId=13 AND SourceEntry=209457)=6
   AND (SELECT COUNT(*) FROM conditions WHERE SourceTypeOrReferenceId=13 AND SourceEntry=209457
    AND SourceGroup=1 AND SourceId=0 AND ElseGroup=0 AND NegativeCondition=0 AND ErrorTextId=0 AND ScriptName=''
    AND (ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2,ConditionValue3) IN
     ((31,0,3,105298,0),(36,0,0,0,0),(22,0,1600,0,0),(44,0,1066,4,0),(31,1,3,104856,0),(36,1,0,0,0)))=6));
SET @sharasdal_carry_guard=IF(COALESCE(@sharasdal_carry_safe,0),'DO 0',
 'SIGNAL SQLSTATE ''45000'' SET MESSAGE_TEXT=''Sharasdal carry actor/implicit-target graph differs; review before updating''');
PREPARE sharasdal_carry_statement FROM @sharasdal_carry_guard;
EXECUTE sharasdal_carry_statement;
DEALLOCATE PREPARE sharasdal_carry_statement;
INSERT INTO conditions
 (SourceTypeOrReferenceId,SourceGroup,SourceEntry,SourceId,ElseGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2,ConditionValue3,NegativeCondition,ErrorTextId,ScriptName,Comment)
SELECT 13,1,209457,0,0,r.kind,r.target,r.value1,r.value2,0,0,0,'','Sharasdal209457: native living Grash carry during1066 finale'
FROM (
 SELECT 31 AS kind,0 AS target,3 AS value1,105298 AS value2
 UNION ALL SELECT 36,0,0,0
 UNION ALL SELECT 22,0,1600,0
 UNION ALL SELECT 44,0,1066,4
 UNION ALL SELECT 31,1,3,104856
 UNION ALL SELECT 36,1,0,0
) r WHERE @sharasdal_carry_safe AND NOT EXISTS
 (SELECT 1 FROM conditions c WHERE c.SourceTypeOrReferenceId=13 AND c.SourceGroup=1 AND c.SourceEntry=209457 AND c.SourceId=0 AND c.ElseGroup=0
  AND c.ConditionTypeOrReference=r.kind AND c.ConditionTarget=r.target AND c.ConditionValue1=r.value1 AND c.ConditionValue2=r.value2 AND c.ConditionValue3=0);
COMMIT;
