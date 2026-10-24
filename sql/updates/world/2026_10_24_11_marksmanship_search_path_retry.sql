START TRANSACTION;

SET @marksman_search_path_retry :=
    EXISTS (SELECT 1 FROM creature_template WHERE entry=100397 AND AIName='SmartAI' AND ScriptName='')
AND EXISTS (SELECT 1 FROM smart_scripts WHERE entryorguid=100397 AND source_type=0 AND id=42 AND link=43
    AND Difficulties='' AND event_type=52 AND event_phase_mask=0 AND event_chance=100 AND event_flags=1
    AND event_param1=16 AND event_param2=100397 AND event_param3=0 AND event_param4=0 AND event_param5=0
    AND action_type=1 AND action_param1=17 AND action_param2=7000 AND action_param3=0 AND action_param4=0 AND action_param5=0 AND action_param6=0
    AND target_type=1 AND target_param1=0 AND target_param2=0 AND target_param3=0 AND target_param4=0)
AND EXISTS (SELECT 1 FROM smart_scripts WHERE entryorguid=100397 AND source_type=0 AND id=43 AND link=0
    AND Difficulties='' AND event_type=61 AND event_phase_mask=0 AND event_chance=100 AND event_flags=1
    AND event_param1=0 AND event_param2=0 AND event_param3=0 AND event_param4=0 AND event_param5=0
    AND action_type=53 AND action_param1=1 AND action_param2=10039704 AND action_param3=0 AND action_param4=0 AND action_param5=0 AND action_param6=0
    AND target_type=1 AND target_param1=0 AND target_param2=0 AND target_param3=0 AND target_param4=0
    AND target_x=0 AND target_y=0 AND target_z=0 AND target_o=0)
AND EXISTS (SELECT 1 FROM smart_scripts WHERE entryorguid=100397 AND source_type=0 AND id=60
    AND event_type=60 AND event_flags=0 AND action_type=45 AND action_param1=10 AND action_param2=10 AND target_type=1)
AND EXISTS (SELECT 1 FROM smart_scripts WHERE entryorguid=100397 AND source_type=0 AND id=44
    AND event_type=40 AND event_param1=7 AND event_param2=10039704 AND action_type=1 AND action_param1=18 AND action_param2=7000)
AND EXISTS (SELECT 1 FROM smart_scripts WHERE entryorguid=100397 AND source_type=0 AND id=45
    AND event_type=40 AND event_param1=14 AND event_param2=10039704 AND action_type=1 AND action_param1=19 AND action_param2=5000)
AND (SELECT COUNT(*) FROM waypoints WHERE entry=10039704)=16
AND EXISTS (SELECT 1 FROM waypoints WHERE entry=10039704 AND pointid=1
    AND ABS(position_x-192.083)<0.001 AND ABS(position_y-1282.94)<0.001 AND ABS(position_z+59.0106)<0.001)
AND EXISTS (SELECT 1 FROM waypoints WHERE entry=10039704 AND pointid=16
    AND ABS(position_x-68.9738)<0.001 AND ABS(position_y-1218.67)<0.001 AND ABS(position_z+39.7261)<0.001)
AND NOT EXISTS (SELECT 1 FROM smart_scripts WHERE entryorguid=100397 AND source_type=0 AND id=61)
AND NOT EXISTS (SELECT 1 FROM smart_scripts WHERE entryorguid=100397 AND source_type=0
    AND (event_phase_mask<>0 OR action_type IN(22,23,30,31)))
AND NOT EXISTS (SELECT 1 FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceEntry=100397 AND SourceGroup IN(43,44,62));

UPDATE smart_scripts SET action_type=22,action_param1=1,action_param2=0,
    comment='Marksmanship: retain pending search path after the original Enough dialogue'
WHERE entryorguid=100397 AND source_type=0 AND id=43 AND @marksman_search_path_retry;

INSERT INTO smart_scripts (entryorguid,source_type,id,link,event_type,event_phase_mask,event_chance,event_flags,
    event_param1,event_param2,event_param3,event_param4,action_type,action_param1,action_param2,target_type,comment)
SELECT 100397,0,61,0,60,1,100,1,0,0,1000,1000,53,1,10039704,1,
    'Marksmanship: retry the armed native overlook search path until movement starts'
WHERE @marksman_search_path_retry;

INSERT INTO conditions (SourceTypeOrReferenceId,SourceGroup,SourceEntry,SourceId,ElseGroup,ConditionTypeOrReference,
    ConditionTarget,ConditionValue1,ConditionValue2,Comment)
SELECT 22,43,100397,0,0,44,1,972,6,'Marksmanship: retain Enough dialogue until Where Is She'
WHERE @marksman_search_path_retry
UNION ALL SELECT 22,43,100397,0,0,36,1,0,0,'Marksmanship: Enough dialogue requires living Vereesa'
WHERE @marksman_search_path_retry
UNION ALL SELECT 22,44,100397,0,0,44,1,972,6,'Marksmanship: arm overlook search only during Where Is She'
WHERE @marksman_search_path_retry
UNION ALL SELECT 22,44,100397,0,0,36,1,0,0,'Marksmanship: arm overlook search only for living Vereesa'
WHERE @marksman_search_path_retry
UNION ALL SELECT 22,62,100397,0,0,44,1,972,6,'Marksmanship: start armed overlook search only during Where Is She'
WHERE @marksman_search_path_retry
UNION ALL SELECT 22,62,100397,0,0,36,1,0,0,'Marksmanship: start armed overlook search only for living Vereesa'
WHERE @marksman_search_path_retry;

COMMIT;
