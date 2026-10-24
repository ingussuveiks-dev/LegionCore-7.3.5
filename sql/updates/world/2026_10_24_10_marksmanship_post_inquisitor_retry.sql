START TRANSACTION;

SET @marksman_search_retry :=
    (SELECT COUNT(*) FROM creature_template WHERE entry IN(100397,101269) AND AIName='SmartAI' AND ScriptName='')=2
AND EXISTS (SELECT 1 FROM smart_scripts WHERE entryorguid=100397 AND source_type=0 AND id=39 AND link=40
    AND Difficulties='' AND event_type=38 AND event_phase_mask=0 AND event_chance=100 AND event_flags=1
    AND event_param1=10 AND event_param2=10 AND event_param3=0 AND event_param4=0 AND event_param5=0
    AND action_type=53 AND action_param1=1 AND action_param2=10039705 AND action_param3=0 AND action_param4=0 AND action_param5=0 AND action_param6=0
    AND target_type=1 AND target_param1=0 AND target_param2=0 AND target_param3=0 AND target_param4=0
    AND target_x=0 AND target_y=0 AND target_z=0 AND target_o=0)
AND EXISTS (SELECT 1 FROM smart_scripts WHERE entryorguid=100397 AND source_type=0 AND id=40 AND link=41
    AND event_type=61 AND action_type=1 AND action_param1=16 AND action_param2=8000 AND target_type=21 AND target_param1=100)
AND EXISTS (SELECT 1 FROM smart_scripts WHERE entryorguid=100397 AND source_type=0 AND id=41 AND link=0
    AND event_type=61 AND action_type=28 AND action_param1=0 AND target_type=1)
AND EXISTS (SELECT 1 FROM smart_scripts WHERE entryorguid=100397 AND source_type=0 AND id=42 AND link=43
    AND event_type=52 AND event_param1=16 AND event_param2=100397 AND action_type=1 AND action_param1=17 AND action_param2=7000 AND target_type=1)
AND EXISTS (SELECT 1 FROM smart_scripts WHERE entryorguid=100397 AND source_type=0 AND id=43 AND link=0
    AND event_type=61 AND action_type=53 AND action_param1=1 AND action_param2=10039704 AND target_type=1)
AND EXISTS (SELECT 1 FROM smart_scripts WHERE entryorguid=101269 AND source_type=0 AND id=10 AND link=11
    AND event_type=6 AND action_type=205 AND action_param1=92 AND action_param2=49654 AND target_type=7)
AND EXISTS (SELECT 1 FROM smart_scripts WHERE entryorguid=101269 AND source_type=0 AND id=11 AND link=12
    AND event_type=61 AND action_type=1 AND action_param1=0 AND target_type=1)
AND EXISTS (SELECT 1 FROM smart_scripts WHERE entryorguid=101269 AND source_type=0 AND id=12 AND link=0
    AND event_type=61 AND action_type=45 AND action_param1=10 AND action_param2=10 AND target_type=19 AND target_param1=100397 AND target_param2=0 AND target_param3=0)
AND EXISTS (SELECT 1 FROM waypoints WHERE entry=10039705 AND pointid=1
    AND ABS(position_x-187.494)<0.001 AND ABS(position_y-1295.63)<0.001 AND ABS(position_z+59.0451)<0.001)
AND (SELECT COUNT(*) FROM waypoints WHERE entry=10039705)=1
AND EXISTS (SELECT 1 FROM creature WHERE guid=370758 AND id=100397 AND map=1489 AND PhaseId=5586)
AND NOT EXISTS (SELECT 1 FROM smart_scripts WHERE entryorguid=100397 AND source_type=0 AND id=60)
AND NOT EXISTS (SELECT 1 FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceEntry=100397 AND SourceGroup IN(40,61));

INSERT INTO smart_scripts (entryorguid,source_type,id,link,event_type,event_phase_mask,event_chance,event_flags,
    event_param1,event_param2,event_param3,event_param4,action_type,action_param1,action_param2,target_type,comment)
SELECT 100397,0,60,0,60,0,100,0,0,0,1000,1000,45,10,10,1,
    'Marksmanship: retry native post-Inquisitor companion signal while Where Is She is active'
WHERE @marksman_search_retry;

INSERT INTO conditions (SourceTypeOrReferenceId,SourceGroup,SourceEntry,SourceId,ElseGroup,ConditionTypeOrReference,
    ConditionTarget,ConditionValue1,ConditionValue2,Comment)
SELECT 22,40,100397,0,0,44,1,972,6,'Marksmanship: post-Inquisitor path only during Where Is She'
WHERE @marksman_search_retry
UNION ALL SELECT 22,40,100397,0,0,36,1,0,0,'Marksmanship: post-Inquisitor path requires living Vereesa'
WHERE @marksman_search_retry
UNION ALL SELECT 22,61,100397,0,0,44,1,972,6,'Marksmanship: retry companion signal only during Where Is She'
WHERE @marksman_search_retry
UNION ALL SELECT 22,61,100397,0,0,36,1,0,0,'Marksmanship: retry companion signal only for living Vereesa'
WHERE @marksman_search_retry;

COMMIT;
