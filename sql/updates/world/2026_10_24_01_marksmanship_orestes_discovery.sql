START TRANSACTION;

SET @marksman_native_discovery := EXISTS (
    SELECT 1 FROM creature_template actor
    JOIN smart_scripts discovery ON discovery.entryorguid=actor.entry AND discovery.source_type=0 AND discovery.id=3
    JOIN smart_scripts speech ON speech.entryorguid=actor.entry AND speech.source_type=0 AND speech.id=4
    WHERE actor.entry=100398 AND actor.AIName='SmartAI' AND actor.ScriptName=''
      AND discovery.event_type=10 AND discovery.event_flags=1 AND discovery.event_phase_mask=0 AND discovery.event_chance=100
      AND discovery.event_param1=1 AND discovery.event_param2=5 AND discovery.event_param3=0 AND discovery.event_param4=0
      AND discovery.link=4 AND discovery.action_type=205 AND discovery.action_param1=92 AND discovery.action_param2=49592
      AND discovery.action_param3=0 AND discovery.action_param4=0 AND discovery.action_param5=0 AND discovery.action_param6=0
      AND ((discovery.target_type=21 AND discovery.target_param1=100) OR (discovery.target_type=7 AND discovery.target_param1=0))
      AND discovery.target_param2=0 AND discovery.target_param3=0
      AND discovery.target_x=0 AND discovery.target_y=0 AND discovery.target_z=0 AND discovery.target_o=0
      AND speech.event_type=61 AND speech.event_flags=1 AND speech.event_phase_mask=0 AND speech.event_chance=100
      AND speech.event_param1=0 AND speech.event_param2=0 AND speech.event_param3=0 AND speech.event_param4=0
      AND speech.link=0 AND speech.action_type=85 AND speech.action_param1=200848 AND speech.action_param2=0
      AND speech.action_param3=0 AND speech.action_param4=0 AND speech.action_param5=0 AND speech.action_param6=0
      AND speech.target_type=7 AND speech.target_param1=0 AND speech.target_param2=0 AND speech.target_param3=0
      AND speech.target_x=0 AND speech.target_y=0 AND speech.target_z=0 AND speech.target_o=0
) AND NOT EXISTS (
    SELECT 1 FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceGroup=4 AND SourceEntry=100398 AND SourceId=0
      AND NOT (ElseGroup=0 AND NegativeCondition=0 AND ScriptName='' AND ConditionValue3=0
               AND ((ConditionTypeOrReference=44 AND ConditionTarget=1 AND ConditionValue1=972 AND ConditionValue2=2
                     AND Comment='Marksmanship: discover Orestes only during Search for Your Allies')
                    OR (ConditionTypeOrReference=32 AND ConditionTarget=0 AND ConditionValue1=16 AND ConditionValue2=0
                        AND Comment='Marksmanship: only the player may discover Orestes')))
);

INSERT INTO conditions (SourceTypeOrReferenceId,SourceGroup,SourceEntry,SourceId,ElseGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2,ConditionValue3,NegativeCondition,ErrorTextId,ScriptName,Comment)
SELECT 22,4,100398,0,0,44,1,972,2,0,0,0,'','Marksmanship: discover Orestes only during Search for Your Allies'
WHERE @marksman_native_discovery AND NOT EXISTS (SELECT 1 FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceGroup=4 AND SourceEntry=100398 AND SourceId=0 AND ConditionTypeOrReference=44);

INSERT INTO conditions (SourceTypeOrReferenceId,SourceGroup,SourceEntry,SourceId,ElseGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2,ConditionValue3,NegativeCondition,ErrorTextId,ScriptName,Comment)
SELECT 22,4,100398,0,0,32,0,16,0,0,0,0,'','Marksmanship: only the player may discover Orestes'
WHERE @marksman_native_discovery AND NOT EXISTS (SELECT 1 FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceGroup=4 AND SourceEntry=100398 AND SourceId=0 AND ConditionTypeOrReference=32);

UPDATE smart_scripts SET target_type=7,target_param1=0
WHERE entryorguid=100398 AND source_type=0 AND id=3 AND @marksman_native_discovery;

COMMIT;
