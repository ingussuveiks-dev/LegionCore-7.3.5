START TRANSACTION;

SET @marksman_bow_summon_retry :=
    EXISTS (SELECT 1 FROM creature_template WHERE entry=100397 AND AIName='SmartAI' AND ScriptName='')
AND EXISTS (SELECT 1 FROM gameobject_template WHERE entry=248419 AND type=3 AND displayId=9806
    AND flags=0 AND Data0=1691 AND Data1=248419 AND Data14=23645 AND Data31=1
    AND AIName='SmartGameObjectAI' AND ScriptName='go_marksmanship_pickup')
AND NOT EXISTS (SELECT 1 FROM gameobject WHERE id=248419)
AND EXISTS (SELECT 1 FROM smart_scripts WHERE entryorguid=100397 AND source_type=0 AND id=50 AND link=0
    AND Difficulties='' AND event_type=61 AND event_phase_mask=0 AND event_chance=100 AND event_flags=1
    AND event_param1=0 AND event_param2=0 AND event_param3=0 AND event_param4=0 AND event_param5=0
    AND action_type=50 AND action_param1=248419 AND action_param2=3600
    AND action_param3=0 AND action_param4=0 AND action_param5=0 AND action_param6=0
    AND target_type=8 AND target_param1=0 AND target_param2=0 AND target_param3=0 AND target_param4=0
    AND ABS(target_x-62.6788)<0.001 AND ABS(target_y-1215.55)<0.001
    AND ABS(target_z+38.6585)<0.001 AND ABS(target_o-0.705752)<0.001)
AND EXISTS (SELECT 1 FROM smart_scripts WHERE entryorguid=100397 AND source_type=0 AND id=45 AND link=50
    AND event_type=40 AND event_flags=1 AND event_param1=14 AND event_param2=10039704
    AND action_type=1 AND action_param1=19 AND action_param2=5000 AND target_type=1)
AND EXISTS (SELECT 1 FROM smart_scripts WHERE entryorguid=100397 AND source_type=0 AND id=49 AND link=51
    AND event_type=61 AND action_type=205 AND action_param1=92 AND action_param2=47846 AND target_type=21 AND target_param1=50)
AND EXISTS (SELECT 1 FROM smart_scripts WHERE entryorguid=100397 AND source_type=0 AND id=43
    AND action_type=22 AND action_param1=1 AND action_param2=0)
AND EXISTS (SELECT 1 FROM smart_scripts WHERE entryorguid=100397 AND source_type=0 AND id=61
    AND event_type=60 AND event_phase_mask=1 AND event_flags=1 AND action_type=53 AND action_param1=1 AND action_param2=10039704)
AND NOT EXISTS (SELECT 1 FROM smart_scripts WHERE entryorguid=100397 AND source_type=0
    AND (id=62 OR event_phase_mask<>0 AND id<>61 OR action_type IN(22,23,30,31) AND id<>43
        OR link=50 AND id<>45 OR action_type=50 AND action_param1=248419 AND id<>50))
AND NOT EXISTS (SELECT 1 FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceEntry=100397 AND SourceGroup=63)
AND (SELECT COUNT(*) FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceEntry=100397 AND SourceGroup=51)=2
AND (SELECT COUNT(*) FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceEntry=100397 AND SourceGroup=51
    AND SourceId=0 AND ElseGroup=0 AND ConditionTarget=1 AND ConditionValue3=0 AND NegativeCondition=0 AND ScriptName=''
    AND (ConditionTypeOrReference=36 AND ConditionValue1=0 AND ConditionValue2=0
        OR ConditionTypeOrReference=44 AND ConditionValue1=972 AND ConditionValue2=6))=2;

INSERT INTO smart_scripts (entryorguid,source_type,id,link,event_type,event_phase_mask,event_chance,event_flags,
    event_param1,event_param2,event_param3,event_param4,action_type,action_param1,action_param2,action_param3,
    target_type,target_x,target_y,target_z,target_o,comment)
SELECT entryorguid,source_type,62,0,60,2,100,1,0,0,1000,1000,action_type,action_param1,action_param2,2,
    target_type,target_x,target_y,target_z,target_o,'Marksmanship: retry the armed bow summon until native creation succeeds'
FROM smart_scripts WHERE entryorguid=100397 AND source_type=0 AND id=50 AND @marksman_bow_summon_retry;

UPDATE smart_scripts SET action_type=22,action_param1=2,action_param2=0,target_type=1,
    target_x=0,target_y=0,target_z=0,target_o=0,comment='Marksmanship: retain pending bow creation after discovery speech'
WHERE entryorguid=100397 AND source_type=0 AND id=50 AND @marksman_bow_summon_retry;

INSERT INTO conditions (SourceTypeOrReferenceId,SourceGroup,SourceEntry,SourceId,ElseGroup,ConditionTypeOrReference,
    ConditionTarget,ConditionValue1,ConditionValue2,Comment)
SELECT 22,63,100397,0,0,44,1,972,6,'Marksmanship: retry bow creation during Where Is She'
WHERE @marksman_bow_summon_retry
UNION ALL SELECT 22,63,100397,0,0,36,1,0,0,'Marksmanship: pending bow creation requires living Vereesa'
WHERE @marksman_bow_summon_retry
UNION ALL SELECT 22,63,100397,0,1,44,1,972,7,'Marksmanship: retain missing bow recovery during A Bow of Legend'
WHERE @marksman_bow_summon_retry
UNION ALL SELECT 22,63,100397,0,1,36,1,0,0,'Marksmanship: pending bow recovery requires living Vereesa'
WHERE @marksman_bow_summon_retry;

COMMIT;
