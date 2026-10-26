-- Allari's original phase must be available after real Arev'naal victory,
-- before meeting Allari completes the still-INCOMPLETE quest. Keep the original
-- not-INCOMPLETE branch and all other Azsuna phase definitions intact.
START TRANSACTION;

SET @scythe_return :=
EXISTS (SELECT 1 FROM phase_definitions WHERE zoneId=7334 AND entry=3 AND phasemask=0 AND phaseId='4265' AND flags=16)
AND EXISTS (SELECT 1 FROM creature WHERE guid=271101 AND id=89398 AND map=1220 AND phaseMask=1 AND PhaseId='4265')
AND EXISTS (SELECT 1 FROM quest_objectives WHERE ID=277159 AND QuestID=37660 AND Type=0 AND ObjectID=89673 AND Amount=1 AND StorageIndex=7)
AND EXISTS (SELECT 1 FROM quest_objectives WHERE ID=286117 AND QuestID=37660 AND Type=0 AND ObjectID=89398 AND Amount=1 AND StorageIndex=9)
AND (SELECT COUNT(*) FROM conditions WHERE SourceTypeOrReferenceId=23 AND SourceGroup=7334 AND SourceEntry=3)=1
AND EXISTS (SELECT 1 FROM conditions WHERE SourceTypeOrReferenceId=23 AND SourceGroup=7334 AND SourceEntry=3 AND SourceId=0 AND ElseGroup=0 AND ConditionTypeOrReference=9 AND ConditionTarget=0 AND ConditionValue1=37660 AND ConditionValue2=0 AND ConditionValue3=0 AND NegativeCondition=1 AND ErrorTextId=0 AND ScriptName='');

INSERT INTO conditions (SourceTypeOrReferenceId,SourceGroup,SourceEntry,SourceId,ElseGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2,ConditionValue3,NegativeCondition,ErrorTextId,ScriptName,Comment)
SELECT 23,7334,3,0,1,9,0,37660,0,0,0,0,'','Scythe of Souls: active quest return to existing Allari after actual victory'
WHERE @scythe_return;
INSERT INTO conditions (SourceTypeOrReferenceId,SourceGroup,SourceEntry,SourceId,ElseGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2,ConditionValue3,NegativeCondition,ErrorTextId,ScriptName,Comment)
SELECT 23,7334,3,0,1,41,0,37660,89673,1,0,0,'','Scythe of Souls: real Arevnaal objective unlocks Allari return phase'
WHERE @scythe_return;

COMMIT;
