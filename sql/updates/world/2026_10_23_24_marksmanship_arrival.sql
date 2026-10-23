START TRANSACTION;

SET @marksman_native_arrival := EXISTS (
    SELECT 1 FROM creature_template actor
    JOIN smart_scripts arrival ON arrival.entryorguid=actor.entry AND arrival.source_type=0 AND arrival.id=0
    JOIN smart_scripts speech ON speech.entryorguid=actor.entry AND speech.source_type=0 AND speech.id=3
    JOIN smart_scripts walk ON walk.entryorguid=actor.entry AND walk.source_type=0 AND walk.id=4
    WHERE actor.entry=100397 AND actor.AIName='SmartAI' AND actor.ScriptName='' AND actor.gossip_menu_id=19331
      AND arrival.event_type=10 AND arrival.event_flags=1 AND arrival.event_phase_mask=0 AND arrival.event_chance=100
      AND arrival.event_param1=1 AND arrival.event_param2=10 AND arrival.event_param3=0 AND arrival.event_param4=0
      AND arrival.link=1 AND arrival.action_type=1 AND arrival.action_param1=0 AND arrival.action_param2=6000
      AND arrival.action_param3=0 AND arrival.action_param4=0 AND arrival.action_param5=0 AND arrival.action_param6=0
      AND arrival.target_type IN(1,7) AND arrival.target_param1=0 AND arrival.target_param2=0 AND arrival.target_param3=0
      AND speech.event_type=52 AND speech.event_param1=0 AND speech.event_param2=100397 AND speech.event_flags=1
      AND speech.link=0 AND speech.action_type=1 AND speech.action_param1=1 AND speech.action_param2=4000 AND speech.target_type=1
      AND walk.event_type=52 AND walk.event_param1=1 AND walk.event_param2=100397 AND walk.event_flags=1
      AND walk.link=0 AND walk.action_type=53 AND walk.action_param1=1 AND walk.action_param2=100397 AND walk.target_type=1
) AND EXISTS (SELECT 1 FROM smart_scripts WHERE entryorguid=100397 AND source_type=0 AND id=1 AND event_type=61 AND link=57 AND action_type=11 AND action_param1=205763 AND target_type=2)
AND EXISTS (SELECT 1 FROM smart_scripts WHERE entryorguid=100397 AND source_type=0 AND id=57 AND event_type=61 AND link=0 AND action_type=33 AND action_param1=102129 AND target_type=7)
AND EXISTS (SELECT 1 FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceGroup=1 AND SourceEntry=100397 AND SourceId=0
            AND ElseGroup=0 AND ConditionTypeOrReference=44 AND ConditionTarget=1 AND ConditionValue1=972 AND ConditionValue2=0 AND ConditionValue3=0 AND NegativeCondition=0 AND ScriptName='')
AND NOT EXISTS (SELECT 1 FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceGroup=1 AND SourceEntry=100397 AND SourceId=0
                AND NOT (ElseGroup=0 AND NegativeCondition=0 AND ScriptName='' AND ConditionValue3=0
                         AND ((ConditionTypeOrReference=44 AND ConditionTarget=1 AND ConditionValue1=972 AND ConditionValue2=0)
                              OR (ConditionTypeOrReference=32 AND ConditionTarget=0 AND ConditionValue1=16 AND ConditionValue2=0
                                  AND Comment='Marksmanship: only the player may start Vereesa arrival'))));

INSERT INTO conditions (SourceTypeOrReferenceId,SourceGroup,SourceEntry,SourceId,ElseGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2,ConditionValue3,NegativeCondition,ErrorTextId,ScriptName,Comment)
SELECT 22,1,100397,0,0,32,0,16,0,0,0,0,'','Marksmanship: only the player may start Vereesa arrival'
WHERE @marksman_native_arrival AND NOT EXISTS (SELECT 1 FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceGroup=1 AND SourceEntry=100397 AND SourceId=0 AND ConditionTypeOrReference=32);

UPDATE smart_scripts SET target_type=1
WHERE entryorguid=100397 AND source_type=0 AND id=0 AND @marksman_native_arrival;

COMMIT;
