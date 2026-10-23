START TRANSACTION;

SET @orestes_credit_retry := EXISTS (
    SELECT 1 FROM creature_template actor
    JOIN smart_scripts start ON start.entryorguid=actor.entry AND start.source_type=0 AND start.id=2
    JOIN smart_scripts credit ON credit.entryorguid=actor.entry AND credit.source_type=9 AND credit.id=0
    JOIN smart_scripts death ON death.entryorguid=actor.entry AND death.source_type=9 AND death.id=1
    WHERE actor.entry=100398 AND actor.AIName='SmartAI' AND actor.ScriptName=''
      AND start.link=0 AND start.event_type=38 AND start.event_phase_mask=0 AND start.event_chance=100 AND start.event_flags=1
      AND start.event_param1=1 AND start.event_param2=1 AND start.event_param3=0 AND start.event_param4=0 AND start.event_param5=0
      AND start.action_type=80 AND start.action_param1=100398 AND start.action_param2=2 AND start.action_param3=0
      AND start.action_param4=0 AND start.action_param5=0 AND start.action_param6=0
      AND start.target_type=1 AND start.target_param1=0 AND start.target_param2=0 AND start.target_param3=0 AND start.target_param4=0
      AND start.target_x=0 AND start.target_y=0 AND start.target_z=0 AND start.target_o=0 AND start.Difficulties=''
      AND credit.link=0 AND credit.event_type=0 AND credit.event_phase_mask=0 AND credit.event_chance=100 AND credit.event_flags=1
      AND credit.event_param1=10000 AND credit.event_param2=10000 AND credit.event_param3=0 AND credit.event_param4=0 AND credit.event_param5=0
      AND credit.action_type=205 AND credit.action_param1=92 AND credit.action_param2=48071 AND credit.action_param3=0
      AND credit.action_param4=0 AND credit.action_param5=0 AND credit.action_param6=0
      AND credit.target_type=21 AND credit.target_param1=50 AND credit.target_param2=0 AND credit.target_param3=0 AND credit.target_param4=0
      AND credit.target_x=0 AND credit.target_y=0 AND credit.target_z=0 AND credit.target_o=0 AND credit.Difficulties=''
      AND death.link=0 AND death.event_type=0 AND death.event_phase_mask=0 AND death.event_chance=100 AND death.event_flags=1
      AND death.event_param1=0 AND death.event_param2=0 AND death.event_param3=0 AND death.event_param4=0 AND death.event_param5=0
      AND death.action_type=37 AND death.action_param1=0 AND death.action_param2=0 AND death.action_param3=0
      AND death.action_param4=0 AND death.action_param5=0 AND death.action_param6=0
      AND death.target_type=1 AND death.target_param1=0 AND death.target_param2=0 AND death.target_param3=0 AND death.target_param4=0
      AND death.target_x=0 AND death.target_y=0 AND death.target_z=0 AND death.target_o=0 AND death.Difficulties=''
) AND (SELECT COUNT(*) FROM smart_scripts WHERE entryorguid=100398 AND source_type=9)=2
  AND NOT EXISTS (SELECT 1 FROM smart_scripts WHERE entryorguid=100398 AND source_type=0
                  AND (id IN(5,6) OR event_phase_mask<>0 OR action_type IN(22,23,30,31)))
  AND NOT EXISTS (SELECT 1 FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceEntry=100398
                  AND ((SourceId=0 AND SourceGroup IN(6,7)) OR SourceId=9));

UPDATE smart_scripts SET action_type=22,action_param1=1,action_param2=0,target_type=1,target_param1=0,
    comment='Orestes - after native farewell delay - wait for an eligible nearby player'
WHERE entryorguid=100398 AND source_type=9 AND id=0 AND @orestes_credit_retry;

DELETE FROM smart_scripts WHERE entryorguid=100398 AND source_type=9 AND id=1 AND @orestes_credit_retry;

INSERT INTO smart_scripts (entryorguid,source_type,id,link,event_type,event_phase_mask,event_chance,event_flags,
    event_param1,event_param2,action_type,action_param1,action_param2,target_type,comment)
SELECT 100398,0,5,6,10,1,100,1,1,50,205,92,48071,7,
    'Orestes - eligible player returns after farewell delay - native scenario credit'
WHERE @orestes_credit_retry;

INSERT INTO smart_scripts (entryorguid,source_type,id,link,event_type,event_chance,action_type,target_type,comment)
SELECT 100398,0,6,0,61,100,37,1,'Orestes - after eligible player credit - native death'
WHERE @orestes_credit_retry;

INSERT INTO conditions (SourceTypeOrReferenceId,SourceGroup,SourceEntry,SourceId,ElseGroup,ConditionTypeOrReference,
    ConditionTarget,ConditionValue1,ConditionValue2,Comment)
SELECT 22,6,100398,0,groups.else_group,rules.kind,rules.target,rules.value1,rules.value2,
    'Marksmanship: Orestes farewell credit requires the present quest player at the Torvis stage'
FROM (SELECT 0 AS else_group UNION ALL SELECT 1) groups
CROSS JOIN (SELECT 32 AS kind,0 AS target,16 AS value1,0 AS value2
            UNION ALL SELECT 44,1,972,3 UNION ALL SELECT 36,0,0,0) rules WHERE @orestes_credit_retry;

INSERT INTO conditions (SourceTypeOrReferenceId,SourceGroup,SourceEntry,SourceId,ElseGroup,ConditionTypeOrReference,
    ConditionTarget,ConditionValue1,Comment)
SELECT 22,6,100398,0,0,9,0,40419,'Marksmanship: Orestes farewell credit requires the active rescue quest'
WHERE @orestes_credit_retry;

INSERT INTO conditions (SourceTypeOrReferenceId,SourceGroup,SourceEntry,SourceId,ElseGroup,ConditionTypeOrReference,
    ConditionTarget,ConditionValue1,Comment)
SELECT 22,6,100398,0,1,28,0,40419,'Marksmanship: also admit a complete but not rewarded rescue quest'
WHERE @orestes_credit_retry;

COMMIT;
