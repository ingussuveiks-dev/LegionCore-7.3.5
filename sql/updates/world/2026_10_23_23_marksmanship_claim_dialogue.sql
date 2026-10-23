START TRANSACTION;

SET @marksman_claim_dialogue := EXISTS (
    SELECT 1 FROM creature_template AS actor
    JOIN smart_scripts AS speech ON speech.entryorguid=actor.entry AND speech.source_type=0 AND speech.id=52
    JOIN smart_scripts AS conversation ON conversation.entryorguid=actor.entry AND conversation.source_type=0 AND conversation.id=53
    WHERE actor.entry=100397 AND actor.AIName='SmartAI' AND actor.ScriptName='' AND actor.gossip_menu_id=19331
      AND speech.link=53 AND speech.event_type=52 AND speech.event_param1=22 AND speech.event_param2=100397
      AND speech.event_flags=1 AND speech.event_phase_mask=0 AND speech.event_chance=100 AND speech.action_type=1
      AND speech.action_param1=23 AND speech.action_param2=5000
      AND speech.action_param3=0 AND speech.action_param4=0 AND speech.action_param5=0 AND speech.action_param6=0
      AND speech.target_param2=0 AND speech.target_param3=0
      AND ((speech.target_type=21 AND speech.target_param1=50) OR (speech.target_type=7 AND speech.target_param1=0))
      AND conversation.link=0 AND conversation.event_type=61 AND conversation.event_flags=1 AND conversation.event_phase_mask=0 AND conversation.event_chance=100 AND conversation.action_type=206
      AND conversation.action_param1=1654
      AND conversation.action_param2=0 AND conversation.action_param3=0 AND conversation.action_param4=0 AND conversation.action_param5=0 AND conversation.action_param6=0
      AND conversation.target_param2=0 AND conversation.target_param3=0
      AND ((conversation.target_type=21 AND conversation.target_param1=50) OR (conversation.target_type=7 AND conversation.target_param1=0))
) AND (SELECT COUNT(*) FROM smart_scripts WHERE entryorguid=100397 AND source_type=0 AND id IN(52,53))=2
AND (SELECT COUNT(*) FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceGroup=60 AND SourceEntry=100397 AND SourceId=0)=12
AND NOT EXISTS (SELECT 1 FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceGroup=53 AND SourceEntry=100397 AND SourceId=0
                AND Comment<>'Marksmanship: claimed scene owner before Vereesa post-acquisition dialogue')
AND (SELECT COUNT(*) FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceGroup=53 AND SourceEntry=100397 AND SourceId=0) IN(0,12);

INSERT INTO conditions (SourceTypeOrReferenceId,SourceGroup,SourceEntry,SourceId,ElseGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2,ConditionValue3,NegativeCondition,ErrorTextId,ScriptName,Comment)
SELECT 22,53,100397,0,ElseGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2,ConditionValue3,NegativeCondition,ErrorTextId,ScriptName,
       'Marksmanship: claimed scene owner before Vereesa post-acquisition dialogue'
FROM conditions
WHERE SourceTypeOrReferenceId=22 AND SourceGroup=60 AND SourceEntry=100397 AND SourceId=0 AND @marksman_claim_dialogue
  AND NOT EXISTS (SELECT 1 FROM (SELECT SourceGroup FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceGroup=53 AND SourceEntry=100397 AND SourceId=0) AS existing);

UPDATE smart_scripts SET target_type=7,target_param1=0
WHERE entryorguid=100397 AND source_type=0 AND id IN(52,53) AND @marksman_claim_dialogue;

COMMIT;
