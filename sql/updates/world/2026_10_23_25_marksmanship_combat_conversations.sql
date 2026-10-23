START TRANSACTION;

UPDATE smart_scripts AS speech
JOIN creature_template actor ON actor.entry=speech.entryorguid AND actor.AIName='SmartAI' AND actor.ScriptName=''
SET speech.target_type=130,speech.target_param1=100
WHERE speech.source_type=0 AND speech.action_type=206
  AND speech.action_param2=0 AND speech.action_param3=0 AND speech.action_param4=0 AND speech.action_param5=0 AND speech.action_param6=0
  AND ((speech.target_type=7 AND speech.target_param1=0) OR (speech.target_type=130 AND speech.target_param1=100))
  AND speech.target_param2=0 AND speech.target_param3=0
  AND speech.event_phase_mask=0 AND speech.event_chance=100
  AND ((speech.entryorguid=100394 AND speech.id=0 AND speech.event_type=4 AND speech.link=2 AND speech.action_param1=1123 AND speech.event_flags=1)
    OR (speech.entryorguid=100749 AND speech.id=4 AND speech.event_type=61 AND speech.link=5 AND speech.action_param1=1009 AND speech.event_flags=0)
    OR (speech.entryorguid=100836 AND speech.id=6 AND speech.event_type=4 AND speech.link=0 AND speech.action_param1=1555 AND speech.event_flags=1)
    OR (speech.entryorguid=100836 AND speech.id=8 AND speech.event_type=61 AND speech.link=0 AND speech.action_param1=1395 AND speech.event_flags=1));

COMMIT;
