START TRANSACTION;

INSERT INTO spell_script_names (spell_id,ScriptName)
SELECT 205040,'spell_marksmanship_return'
WHERE NOT EXISTS (SELECT 1 FROM spell_script_names WHERE spell_id=205040 AND ScriptName='spell_marksmanship_return');

UPDATE smart_scripts AS travel
JOIN creature_template AS actor ON actor.entry=travel.entryorguid
SET travel.action_type=11,
    travel.comment='Link - Vereesa casts native Dalaran Portal Stone on the invoker'
WHERE travel.entryorguid=100397 AND travel.source_type=0 AND travel.id=56
  AND travel.link=0 AND travel.event_type=61 AND travel.action_type=85
  AND travel.action_param1=205040 AND travel.action_param2=0 AND travel.action_param3=0
  AND travel.action_param4=0 AND travel.action_param5=0 AND travel.action_param6=0
  AND travel.target_type=7 AND travel.target_param1=0 AND travel.target_param2=0 AND travel.target_param3=0
  AND actor.AIName='SmartAI' AND actor.ScriptName='' AND actor.gossip_menu_id=19331
  AND EXISTS (SELECT 1 FROM (SELECT * FROM smart_scripts) AS choice
              WHERE choice.entryorguid=100397 AND choice.source_type=0 AND choice.id=59
                AND choice.event_type=62 AND choice.event_param1=19331 AND choice.event_param2=0
                AND choice.link=56 AND choice.action_type=33 AND choice.action_param1=103481
                AND choice.target_type=7 AND (choice.event_flags & 1)=0)
  AND (SELECT COUNT(*) FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceGroup=60
        AND SourceEntry=100397 AND SourceId=0)=12
  AND (SELECT COUNT(*) FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceGroup=60
        AND SourceEntry=100397 AND SourceId=0 AND ConditionTarget=0
        AND ConditionTypeOrReference=44 AND ConditionValue1=972 AND ConditionValue2=8
        AND NegativeCondition=0)=2;

COMMIT;
