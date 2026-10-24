START TRANSACTION;

SET @marksman_bow_reveal :=
    EXISTS (SELECT 1 FROM creature_template WHERE entry=100397 AND AIName='SmartAI' AND ScriptName='')
AND EXISTS (SELECT 1 FROM gameobject_template WHERE entry=248419 AND type=3 AND displayId=9806
    AND flags=0 AND Data0=1691 AND Data1=248419 AND Data14=23645 AND Data31=1
    AND AIName='SmartGameObjectAI' AND ScriptName='go_marksmanship_pickup')
AND NOT EXISTS (SELECT 1 FROM gameobject WHERE id=248419)
AND (SELECT COUNT(*) FROM smart_scripts WHERE entryorguid=100397 AND source_type=0 AND id IN(45,49,50,51)
    AND Difficulties='' AND event_phase_mask=0 AND event_chance=100 AND event_flags=1
    AND event_param3=0 AND event_param4=0 AND event_param5=0
    AND action_param3=0 AND action_param4=0 AND action_param5=0 AND action_param6=0
    AND target_param2=0 AND target_param3=0 AND target_param4=0
    AND (id=50 OR target_x=0 AND target_y=0 AND target_z=0 AND target_o=0)
    AND ((id=45 AND link=0 AND event_type=40 AND event_param1=14 AND event_param2=10039704
        AND action_type=1 AND action_param1=19 AND action_param2=5000 AND target_type=1 AND target_param1=0)
    OR (id=49 AND link=50 AND event_type=61 AND event_param1=0 AND event_param2=0
        AND action_type=205 AND action_param1=92 AND action_param2=47846 AND target_type=21 AND target_param1=50)
    OR (id=50 AND link=51 AND event_type=61 AND event_param1=0 AND event_param2=0
        AND action_type=50 AND action_param1=248419 AND action_param2=3600 AND target_type=8 AND target_param1=0
        AND ABS(target_x-62.6788)<0.001 AND ABS(target_y-1215.55)<0.001
        AND ABS(target_z+38.6585)<0.001 AND ABS(target_o-0.705752)<0.001)
    OR (id=51 AND link=0 AND event_type=61 AND event_param1=0 AND event_param2=0
        AND action_type=33 AND action_param1=102131 AND action_param2=0 AND target_type=21 AND target_param1=50)))=4
AND EXISTS (SELECT 1 FROM smart_scripts WHERE entryorguid=100397 AND source_type=0 AND id=48 AND link=49
    AND event_type=52 AND event_param1=21 AND event_param2=100397
    AND action_type=1 AND action_param1=22 AND action_param2=15000)
AND (SELECT COUNT(*) FROM waypoints WHERE entry=10039704)=16
AND NOT EXISTS (SELECT 1 FROM smart_scripts WHERE entryorguid=100397 AND source_type=0
    AND (link=50 AND id<>49 OR action_type=50 AND action_param1=248419 AND id<>50))
AND NOT EXISTS (SELECT 1 FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceEntry=100397 AND SourceGroup IN(46,51));

UPDATE smart_scripts SET link=50 WHERE entryorguid=100397 AND source_type=0 AND id=45 AND @marksman_bow_reveal;
UPDATE smart_scripts SET link=51 WHERE entryorguid=100397 AND source_type=0 AND id=49 AND @marksman_bow_reveal;
UPDATE smart_scripts SET link=0 WHERE entryorguid=100397 AND source_type=0 AND id=50 AND @marksman_bow_reveal;

INSERT INTO conditions (SourceTypeOrReferenceId,SourceGroup,SourceEntry,SourceId,ElseGroup,ConditionTypeOrReference,
    ConditionTarget,ConditionValue1,ConditionValue2,Comment)
SELECT 22,46,100397,0,0,44,1,972,6,'Marksmanship: discover the bow only during Where Is She'
WHERE @marksman_bow_reveal
UNION ALL SELECT 22,46,100397,0,0,36,1,0,0,'Marksmanship: bow discovery requires living Vereesa'
WHERE @marksman_bow_reveal
UNION ALL SELECT 22,51,100397,0,0,44,1,972,6,'Marksmanship: reveal the bow before the existing pickup stage'
WHERE @marksman_bow_reveal
UNION ALL SELECT 22,51,100397,0,0,36,1,0,0,'Marksmanship: bow reveal requires living Vereesa'
WHERE @marksman_bow_reveal;

COMMIT;
