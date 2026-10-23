START TRANSACTION;

SET @marksman_farewell_door := EXISTS (
    SELECT 1 FROM creature_template WHERE entry IN(100398,100749) AND AIName='SmartAI' AND ScriptName=''
) AND (SELECT COUNT(*) FROM creature_template WHERE entry IN(100398,100749) AND AIName='SmartAI' AND ScriptName='')=2
AND EXISTS (SELECT 1 FROM smart_scripts WHERE entryorguid=100749 AND source_type=0 AND id=2 AND link=3
    AND event_type=61 AND action_type=85 AND action_param1=200832 AND action_param2=128 AND target_type=130 AND target_param1=50)
AND EXISTS (SELECT 1 FROM smart_scripts WHERE entryorguid=100749 AND source_type=0 AND id=3 AND link=4
    AND event_type=61 AND event_phase_mask=0 AND event_flags=0 AND event_chance=100 AND Difficulties=''
    AND event_param1=0 AND event_param2=0 AND event_param3=0 AND event_param4=0 AND event_param5=0
    AND action_type=9 AND action_param1=0 AND action_param2=0 AND action_param3=0 AND action_param4=0 AND action_param5=0 AND action_param6=0
    AND target_type=15 AND target_param1=249740 AND target_param2=50 AND target_param3=0 AND target_param4=0
    AND target_x=0 AND target_y=0 AND target_z=0 AND target_o=0)
AND EXISTS (SELECT 1 FROM smart_scripts WHERE entryorguid=100398 AND source_type=0 AND id=5 AND link=6
    AND event_type=10 AND event_phase_mask=1 AND event_flags=1 AND event_chance=100
    AND event_param1=1 AND event_param2=50 AND action_type=205 AND action_param1=92 AND action_param2=48071
    AND target_type=7 AND target_param1=0 AND target_param2=0)
AND EXISTS (SELECT 1 FROM smart_scripts WHERE entryorguid=100398 AND source_type=0 AND id=6 AND link=0
    AND event_type=61 AND event_phase_mask=0 AND event_flags=0 AND event_chance=100
    AND action_type=37 AND target_type=1)
AND EXISTS (SELECT 1 FROM smart_scripts WHERE entryorguid=100398 AND source_type=9 AND id=0
    AND event_param1=50500 AND event_param2=50500 AND action_type=22 AND action_param1=1)
AND (SELECT COUNT(*) FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceEntry=100398 AND SourceId=0 AND SourceGroup=6)=8
AND NOT EXISTS (SELECT 1 FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceEntry=100398 AND SourceId=0 AND SourceGroup=6
    AND NOT (NegativeCondition=0 AND ScriptName='' AND ConditionValue3=0 AND ElseGroup IN(0,1)
        AND ((ConditionTypeOrReference=32 AND ConditionTarget=0 AND ConditionValue1=16 AND ConditionValue2=0)
          OR (ConditionTypeOrReference=36 AND ConditionTarget=0 AND ConditionValue1=0 AND ConditionValue2=0)
          OR (ConditionTypeOrReference=44 AND ConditionTarget=1 AND ConditionValue1=972 AND ConditionValue2=3)
          OR (ConditionTypeOrReference=9 AND ElseGroup=0 AND ConditionTarget=0 AND ConditionValue1=40419 AND ConditionValue2=0)
          OR (ConditionTypeOrReference=28 AND ElseGroup=1 AND ConditionTarget=0 AND ConditionValue1=40419 AND ConditionValue2=0))))
AND EXISTS (SELECT 1 FROM gameobject_template WHERE entry=249740 AND type=0 AND displayId=26055
    AND flags=16 AND Data0=0 AND Data1=0 AND AIName='' AND ScriptName='')
AND EXISTS (SELECT 1 FROM gameobject WHERE guid=136676 AND id=249740 AND map=1489 AND PhaseId=5586 AND state=1)
AND NOT EXISTS (SELECT 1 FROM smart_scripts WHERE entryorguid=100398 AND source_type=0 AND id=7)
AND NOT EXISTS (SELECT 1 FROM smart_scripts WHERE entryorguid=100749 AND source_type=0 AND id<>2 AND link=3)
AND NOT EXISTS (SELECT 1 FROM conditions WHERE SourceTypeOrReferenceId=22
    AND ((SourceEntry=100398 AND SourceId=0 AND (SourceGroup=8 OR (SourceGroup=6 AND ConditionTypeOrReference=30)))
      OR (SourceEntry=100749 AND SourceId=0 AND SourceGroup=4)));

INSERT INTO smart_scripts (entryorguid,source_type,id,link,Difficulties,event_type,event_phase_mask,event_chance,event_flags,
    event_param1,event_param2,event_param3,event_param4,event_param5,action_type,action_param1,action_param2,
    action_param3,action_param4,action_param5,action_param6,target_type,target_param1,target_param2,target_param3,
    target_param4,target_x,target_y,target_z,target_o,comment)
SELECT 100398,0,7,0,Difficulties,event_type,event_phase_mask,event_chance,event_flags,
    event_param1,event_param2,event_param3,event_param4,event_param5,action_type,action_param1,action_param2,
    action_param3,action_param4,action_param5,action_param6,target_type,target_param1,target_param2,target_param3,
    target_param4,target_x,target_y,target_z,target_o,'Orestes - after eligible farewell credit and death - open native Fel Spike Door'
FROM smart_scripts WHERE entryorguid=100749 AND source_type=0 AND id=3 AND @marksman_farewell_door;

UPDATE smart_scripts SET link=4 WHERE entryorguid=100749 AND source_type=0 AND id=2 AND @marksman_farewell_door;
DELETE FROM smart_scripts WHERE entryorguid=100749 AND source_type=0 AND id=3 AND @marksman_farewell_door;
UPDATE smart_scripts SET link=7 WHERE entryorguid=100398 AND source_type=0 AND id=6 AND @marksman_farewell_door;

INSERT INTO conditions (SourceTypeOrReferenceId,SourceGroup,SourceEntry,SourceId,ElseGroup,ConditionTypeOrReference,
    ConditionTarget,ConditionValue1,ConditionValue2,Comment)
SELECT 22,6,100398,0,groups.else_group,30,1,249740,50,
    'Marksmanship: retain the farewell retry until its native exit door is present'
FROM (SELECT 0 AS else_group UNION ALL SELECT 1) groups WHERE @marksman_farewell_door;

COMMIT;
