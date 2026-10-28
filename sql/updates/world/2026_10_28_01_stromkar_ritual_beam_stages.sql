START TRANSACTION;
SET @stromkar_beam_stages :=
    EXISTS(SELECT 1 FROM creature_template WHERE entry=101875 AND AIName='SmartAI' AND ScriptName='')
    AND EXISTS(SELECT 1 FROM smart_scripts WHERE entryorguid=101875 AND source_type=0 AND id=6 AND link=0
        AND Difficulties='' AND event_type=60 AND event_phase_mask=0 AND event_chance=100 AND event_flags=1
        AND event_param1=1 AND event_param2=70 AND event_param3=0 AND event_param4=0 AND event_param5=0
        AND action_type=11 AND action_param1=201032 AND action_param2=0 AND action_param3=0
        AND action_param4=0 AND action_param5=0 AND action_param6=0
        AND target_type=19 AND target_param1=103144 AND target_param2=50 AND target_param3=0 AND target_param4=0
        AND target_x=0 AND target_y=0 AND target_z=0 AND target_o=0)
    AND (SELECT COUNT(*) FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceGroup=7 AND SourceEntry=101875 AND SourceId=0)=2
    AND (SELECT COUNT(*) FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceGroup=7 AND SourceEntry=101875 AND SourceId=0
        AND ElseGroup=0 AND ConditionTypeOrReference=44 AND ConditionTarget=1 AND ConditionValue1=1037
        AND ConditionValue2 IN(0,1) AND ConditionValue3=0 AND NegativeCondition=0 AND ErrorTextId=0 AND ScriptName='')=2
    AND EXISTS(SELECT 1 FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceGroup=7 AND SourceEntry=101875 AND SourceId=0
        AND ElseGroup=0 AND ConditionTypeOrReference=44 AND ConditionTarget=1 AND ConditionValue1=1037 AND ConditionValue2=0)
    AND EXISTS(SELECT 1 FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceGroup=7 AND SourceEntry=101875 AND SourceId=0
        AND ElseGroup=0 AND ConditionTypeOrReference=44 AND ConditionTarget=1 AND ConditionValue1=1037 AND ConditionValue2=1);

-- Either native early stage, not the impossible simultaneous stage0 AND stage1.
-- Source22 group7 is event6; its target1 is the actual ritualist base actor.
UPDATE conditions SET ElseGroup=1
WHERE SourceTypeOrReferenceId=22 AND SourceGroup=7 AND SourceEntry=101875 AND SourceId=0
    AND ElseGroup=0 AND ConditionTypeOrReference=44 AND ConditionTarget=1
    AND ConditionValue1=1037 AND ConditionValue2=0 AND ConditionValue3=0
    AND NegativeCondition=0 AND ErrorTextId=0 AND ScriptName='' AND @stromkar_beam_stages;
COMMIT;
