-- Native26972 Recklessness122354/125873 effects0+1(normal)/0+1+2(heroic) use nearby-entry38.
-- Both debuffs belong to Wind Lord Meljarak62397,including a player/pet
-- standing inside his model. An unconstrained nearest unit can steal them.
SET @meljarak_target_guard = IF(
 EXISTS(SELECT 1 FROM conditions WHERE SourceTypeOrReferenceId=13 AND SourceEntry IN(122354,125873) AND NOT
  (SourceGroup=IF(SourceEntry=122354,3,7) AND SourceId=0 AND ElseGroup=0 AND ConditionTypeOrReference=31 AND ConditionTarget=0 AND ConditionValue1=3 AND ConditionValue2=62397 AND ConditionValue3=0 AND NegativeCondition=0 AND ErrorTextId=0 AND ScriptName='')) OR
 EXISTS(SELECT 1 FROM conditions WHERE SourceTypeOrReferenceId=13 AND SourceEntry IN(122354,125873) GROUP BY SourceEntry HAVING COUNT(*)>1),
 'SIGNAL SQLSTATE ''45000'' SET MESSAGE_TEXT = ''Meljarak Recklessness has unexpected target conditions: review before repair''','DO 0');
PREPARE meljarak_target_guard FROM @meljarak_target_guard;
EXECUTE meljarak_target_guard;
DEALLOCATE PREPARE meljarak_target_guard;
INSERT INTO conditions (SourceTypeOrReferenceId,SourceGroup,SourceEntry,SourceId,ElseGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2,ConditionValue3,NegativeCondition,ErrorTextId,ScriptName,Comment)
SELECT 13,3,122354,0,0,31,0,3,62397,0,0,0,'','Recklessness: actual Wind Lord Meljarak only'
WHERE NOT EXISTS(SELECT 1 FROM conditions WHERE SourceTypeOrReferenceId=13 AND SourceEntry=122354);
INSERT INTO conditions (SourceTypeOrReferenceId,SourceGroup,SourceEntry,SourceId,ElseGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2,ConditionValue3,NegativeCondition,ErrorTextId,ScriptName,Comment)
SELECT 13,7,125873,0,0,31,0,3,62397,0,0,0,'','Heroic Recklessness: actual Wind Lord Meljarak only'
WHERE NOT EXISTS(SELECT 1 FROM conditions WHERE SourceTypeOrReferenceId=13 AND SourceEntry=125873);
SET @meljarak_target_guard = NULL;
