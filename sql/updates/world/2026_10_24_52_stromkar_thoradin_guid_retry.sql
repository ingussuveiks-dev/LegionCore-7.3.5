START TRANSACTION;

UPDATE conditions current
JOIN creature_template actor ON actor.entry=104276 AND actor.AIName='SmartAI' AND actor.ScriptName='npc_stromkar_zakajz'
JOIN smart_scripts retry ON retry.entryorguid=104276 AND retry.source_type=0 AND retry.id=9 AND retry.event_type=60
    AND retry.event_phase_mask=0 AND retry.event_flags=0 AND retry.event_param3=500 AND retry.event_param4=500
    AND retry.action_type=12 AND retry.action_param1=104307 AND retry.action_param2=8 AND retry.action_param3=0 AND retry.target_type=8
SET current.ScriptName='condition_stromkar_thoradin_absence'
WHERE current.SourceTypeOrReferenceId=22 AND current.SourceGroup=10 AND current.SourceEntry=104276 AND current.SourceId=0 AND current.ElseGroup=0
    AND current.ConditionTypeOrReference=29 AND current.ConditionTarget=1 AND current.ConditionValue1=104307 AND current.ConditionValue2=50
    AND current.ConditionValue3=0 AND current.NegativeCondition=1 AND current.ErrorTextId=0 AND current.ScriptName='';

COMMIT;
