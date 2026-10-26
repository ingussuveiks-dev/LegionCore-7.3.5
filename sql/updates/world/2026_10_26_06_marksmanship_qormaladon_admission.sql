START TRANSACTION;

SET @qormaladon_admission := EXISTS (SELECT 1 FROM creature_template WHERE entry=101269 AND AIName='SmartAI' AND ScriptName='' AND unit_flags=256)
AND (SELECT COUNT(*) FROM creature WHERE id=101269)=1
AND EXISTS (SELECT 1 FROM creature WHERE guid=370739 AND id=101269 AND map=1489 AND PhaseId='5586')
AND EXISTS (SELECT 1 FROM smart_scripts WHERE entryorguid=101269 AND source_type=0 AND id=1 AND link=0 AND Difficulties='' AND event_type=38 AND event_flags=1 AND event_param1=1 AND event_param2=1 AND action_type=19 AND action_param1=256 AND target_type=1)
AND EXISTS (SELECT 1 FROM smart_scripts WHERE entryorguid=101269 AND source_type=0 AND id=10 AND link=11 AND Difficulties='' AND event_type=6 AND event_flags=1 AND action_type=205 AND action_param1=92 AND action_param2=49654 AND target_type=7)
AND EXISTS (SELECT 1 FROM smart_scripts WHERE entryorguid=101269 AND source_type=0 AND id=11 AND link=12 AND event_type=61 AND action_type=1 AND action_param1=0 AND target_type=1)
AND EXISTS (SELECT 1 FROM smart_scripts WHERE entryorguid=101269 AND source_type=0 AND id=12 AND link=0 AND event_type=61 AND action_type=45 AND action_param1=10 AND action_param2=10 AND target_type=19 AND target_param1=100397)
AND EXISTS (SELECT 1 FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceGroup=1 AND SourceEntry=101269 AND SourceId=0 AND ConditionTypeOrReference=44 AND ConditionTarget=1 AND ConditionValue1=972 AND ConditionValue2=5 AND NegativeCondition=0)
AND NOT EXISTS (SELECT 1 FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceGroup IN (2,11) AND SourceEntry=101269 AND SourceId=0);

UPDATE creature_template SET ScriptName='npc_marksmanship_qormaladon'
WHERE entry=101269 AND @qormaladon_admission;

INSERT INTO conditions (SourceTypeOrReferenceId,SourceGroup,SourceEntry,SourceId,ElseGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2,ConditionValue3,NegativeCondition,ErrorTextId,ScriptName,Comment)
SELECT 22,2,101269,0,0,44,1,972,5,0,0,0,'','Marksmanship: native Qormaladon unlock signal only in End the Inquisition'
WHERE @qormaladon_admission;
INSERT INTO conditions (SourceTypeOrReferenceId,SourceGroup,SourceEntry,SourceId,ElseGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2,ConditionValue3,NegativeCondition,ErrorTextId,ScriptName,Comment)
SELECT 22,11,101269,0,0,44,1,972,5,0,0,0,'','Marksmanship: native Qormaladon death only in End the Inquisition'
WHERE @qormaladon_admission;
INSERT INTO conditions (SourceTypeOrReferenceId,SourceGroup,SourceEntry,SourceId,ElseGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2,ConditionValue3,NegativeCondition,ErrorTextId,ScriptName,Comment)
SELECT 22,11,101269,0,0,31,1,3,101269,0,0,0,'condition_marksmanship_qormaladon_ready','Marksmanship: direct-death path also requires the actual interrogation immunity removal'
WHERE @qormaladon_admission;

COMMIT;
