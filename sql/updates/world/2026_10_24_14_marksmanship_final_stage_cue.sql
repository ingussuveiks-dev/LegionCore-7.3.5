START TRANSACTION;

SET @marksman_final_stage_cue :=
    EXISTS (SELECT 1 FROM creature_template WHERE entry=100397 AND AIName='SmartAI' AND ScriptName='')
AND (SELECT COUNT(*) FROM smart_scripts WHERE entryorguid=100397 AND source_type=0 AND id IN(48,49,51)
    AND Difficulties='' AND event_phase_mask=0 AND event_chance=100 AND event_flags=1
    AND event_param3=0 AND event_param4=0 AND event_param5=0
    AND action_param3=0 AND action_param4=0 AND action_param5=0 AND action_param6=0
    AND target_type=21 AND target_param1=50 AND target_param2=0 AND target_param3=0 AND target_param4=0
    AND target_x=0 AND target_y=0 AND target_z=0 AND target_o=0
    AND ((id=48 AND link=49 AND event_type=52 AND event_param1=21 AND event_param2=100397
        AND action_type=1 AND action_param1=22 AND action_param2=15000)
    OR (id=49 AND link=51 AND event_type=61 AND event_param1=0 AND event_param2=0
        AND action_type=205 AND action_param1=92 AND action_param2=47846)
    OR (id=51 AND link=0 AND event_type=61 AND event_param1=0 AND event_param2=0
        AND action_type=33 AND action_param1=102131 AND action_param2=0)))=3
AND EXISTS (SELECT 1 FROM smart_scripts WHERE entryorguid=100397 AND source_type=0 AND id=43
    AND action_type=22 AND action_param1=1 AND action_param2=0)
AND EXISTS (SELECT 1 FROM smart_scripts WHERE entryorguid=100397 AND source_type=0 AND id=50
    AND action_type=22 AND action_param1=2 AND action_param2=0 AND target_type=1)
AND EXISTS (SELECT 1 FROM smart_scripts WHERE entryorguid=100397 AND source_type=0 AND id=61
    AND event_phase_mask=1 AND action_type=53 AND action_param1=1 AND action_param2=10039704)
AND EXISTS (SELECT 1 FROM smart_scripts WHERE entryorguid=100397 AND source_type=0 AND id=62 AND link=0
    AND Difficulties='' AND event_type=60 AND event_phase_mask=2 AND event_chance=100 AND event_flags=1
    AND event_param1=0 AND event_param2=0 AND event_param3=1000 AND event_param4=1000 AND event_param5=0
    AND action_type=50 AND action_param1=248419 AND action_param2=3600 AND action_param3=2
    AND action_param4=0 AND action_param5=0 AND action_param6=0 AND target_type=8
    AND target_param1=0 AND target_param2=0 AND target_param3=0 AND target_param4=0
    AND ABS(target_x-62.6788)<0.001 AND ABS(target_y-1215.55)<0.001
    AND ABS(target_z+38.6585)<0.001 AND ABS(target_o-0.705752)<0.001)
AND NOT EXISTS (SELECT 1 FROM smart_scripts WHERE entryorguid=100397 AND source_type=0
    AND (id=63 OR event_phase_mask<>0 AND id NOT IN(61,62)
        OR action_type IN(22,23,30,31) AND id NOT IN(43,50) OR link=49 AND id<>48))
AND NOT EXISTS (SELECT 1 FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceEntry=100397 AND SourceGroup IN(50,52,64));

INSERT INTO smart_scripts (entryorguid,source_type,id,link,event_type,event_phase_mask,event_chance,event_flags,
    event_param1,event_param2,event_param3,event_param4,action_type,action_param1,action_param2,
    target_type,target_param1,comment)
SELECT entryorguid,source_type,63,link,60,4,100,0,0,0,1000,1000,action_type,action_param1,action_param2,
    target_type,target_param1,'Marksmanship: retain final-stage transition after Take dialogue until a player is nearby'
FROM smart_scripts WHERE entryorguid=100397 AND source_type=0 AND id=49 AND @marksman_final_stage_cue;

UPDATE smart_scripts SET link=0 WHERE entryorguid=100397 AND source_type=0 AND id=48 AND @marksman_final_stage_cue;
UPDATE smart_scripts SET link=0,event_type=52,event_param1=22,event_param2=100397,
    action_type=22,action_param1=3,action_param2=0,target_type=1,target_param1=0,
    comment='Marksmanship: arm final-stage permission only after the native Take dialogue timer'
WHERE entryorguid=100397 AND source_type=0 AND id=49 AND @marksman_final_stage_cue;
UPDATE smart_scripts SET event_phase_mask=6 WHERE entryorguid=100397 AND source_type=0 AND id=62 AND @marksman_final_stage_cue;

INSERT INTO conditions (SourceTypeOrReferenceId,SourceGroup,SourceEntry,SourceId,ElseGroup,ConditionTypeOrReference,
    ConditionTarget,ConditionValue1,ConditionValue2,Comment)
SELECT 22,50,100397,0,0,44,1,972,6,'Marksmanship: Take dialogue expiry belongs to Where Is She'
WHERE @marksman_final_stage_cue
UNION ALL SELECT 22,64,100397,0,0,44,1,972,6,'Marksmanship: pending final-stage progression stops after actual native advancement'
WHERE @marksman_final_stage_cue
UNION ALL SELECT 22,64,100397,0,0,36,1,0,0,'Marksmanship: pending final-stage progression requires living Vereesa'
WHERE @marksman_final_stage_cue
UNION ALL SELECT 22,52,100397,0,0,44,1,972,7,'Marksmanship: native search credit follows successful final-stage advancement'
WHERE @marksman_final_stage_cue
UNION ALL SELECT 22,52,100397,0,0,36,1,0,0,'Marksmanship: native search credit requires living Vereesa'
WHERE @marksman_final_stage_cue;

COMMIT;
