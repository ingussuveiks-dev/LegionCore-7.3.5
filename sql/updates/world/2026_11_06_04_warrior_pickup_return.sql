-- CODE-COUPLED: promote only with WarriorPickupReturn/condition_warrior_pickup_return.
-- Native custom11869/215997/192085 and all Stromkar admission branches remain.
START TRANSACTION;
SET @warrior_exit_safe :=
 (SELECT COUNT(*) FROM creature_template WHERE entry=93819 AND AIName='SmartAI' AND ScriptName='npc_stromkar_valkyr')=1
 AND (SELECT COUNT(*) FROM areatrigger_template WHERE customEntry=11869 AND entry=0 AND VisualID=230829 AND Radius=5 AND RadiusTarget=5 AND isMoving=0)=1
 AND (SELECT COUNT(*) FROM areatrigger_actions WHERE customEntry=11869)=2
 AND (SELECT COUNT(*) FROM areatrigger_actions WHERE customEntry=11869 AND entry=0 AND targetFlags=8 AND spellId=215997
      AND ((id=0 AND moment=1 AND actionType=0) OR (id=1 AND moment=42 AND actionType=1)))=2
 AND (SELECT COUNT(*) FROM conditions WHERE SourceTypeOrReferenceId=26 AND SourceGroup=11869 AND SourceEntry=0 AND ElseGroup=0)=1
 AND (SELECT COUNT(*) FROM conditions WHERE SourceTypeOrReferenceId=26 AND SourceGroup=11869 AND SourceEntry=0 AND SourceId=0 AND ElseGroup=0
      AND ConditionTypeOrReference=22 AND ConditionTarget=0 AND ConditionValue1=1539 AND ConditionValue2=0 AND ConditionValue3=0 AND NegativeCondition=1
      AND ErrorTextId=0 AND ScriptName IN ('','condition_warrior_pickup_return'))=1
 AND ((SELECT COUNT(*) FROM conditions WHERE SourceTypeOrReferenceId=26 AND SourceGroup=11869 AND SourceEntry=1)=0
 OR ((SELECT COUNT(*) FROM conditions WHERE SourceTypeOrReferenceId=26 AND SourceGroup=11869 AND SourceEntry=1)=2
 AND (SELECT COUNT(*) FROM conditions WHERE SourceTypeOrReferenceId=26 AND SourceGroup=11869 AND SourceEntry=1 AND SourceId=0
      AND ConditionTypeOrReference=22 AND ConditionTarget=0 AND ConditionValue1=1539 AND ConditionValue2=0 AND ConditionValue3=0 AND ErrorTextId=0
      AND ((ElseGroup=0 AND NegativeCondition=1 AND ScriptName='condition_warrior_pickup_return') OR (ElseGroup=1 AND NegativeCondition=0 AND ScriptName='')))=2));
SET @warrior_exit_check := IF(COALESCE(@warrior_exit_safe,0),'DO 0','SIGNAL SQLSTATE ''45000'' SET MESSAGE_TEXT=''Warrior return actor/shared gaze predicates changed; review before promotion''');
PREPARE warrior_exit_check FROM @warrior_exit_check;EXECUTE warrior_exit_check;DEALLOCATE PREPARE warrior_exit_check;
UPDATE conditions SET ScriptName='condition_warrior_pickup_return'
 WHERE SourceTypeOrReferenceId=26 AND SourceGroup=11869 AND SourceEntry=0 AND SourceId=0 AND ElseGroup=0
 AND ConditionTypeOrReference=22 AND ConditionTarget=0 AND ConditionValue1=1539 AND NegativeCondition=1 AND ScriptName='';
INSERT INTO conditions(SourceTypeOrReferenceId,SourceGroup,SourceEntry,SourceId,ElseGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2,ConditionValue3,NegativeCondition,ErrorTextId,ScriptName,Comment)
SELECT 26,11869,1,0,0,22,0,1539,0,0,1,0,'condition_warrior_pickup_return','Warrior: native leave removes gaze only from this scoped return owner; ordinary outside-tomb behavior retained'
WHERE NOT EXISTS (SELECT 1 FROM conditions WHERE SourceTypeOrReferenceId=26 AND SourceGroup=11869 AND SourceEntry=1 AND ElseGroup=0);
INSERT INTO conditions(SourceTypeOrReferenceId,SourceGroup,SourceEntry,SourceId,ElseGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2,ConditionValue3,NegativeCondition,ErrorTextId,ScriptName,Comment)
SELECT 26,11869,1,0,1,22,0,1539,0,0,0,0,'','Warrior: preserve original unconditional native Stromkar leave cleanup'
WHERE NOT EXISTS (SELECT 1 FROM conditions WHERE SourceTypeOrReferenceId=26 AND SourceGroup=11869 AND SourceEntry=1 AND ElseGroup=1);
COMMIT;
