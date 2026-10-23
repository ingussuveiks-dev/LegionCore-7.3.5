START TRANSACTION;

UPDATE smart_scripts spell
JOIN creature_template actor ON actor.entry=spell.entryorguid AND actor.AIName='SmartAI' AND actor.ScriptName=''
SET spell.action_param2=128,spell.target_type=130,spell.target_param1=50
WHERE spell.entryorguid=100749 AND spell.source_type=0 AND spell.id=2 AND spell.link=3
  AND spell.event_type=61 AND spell.event_phase_mask=0 AND spell.event_chance=100 AND spell.event_flags=0
  AND spell.event_param1=0 AND spell.event_param2=0 AND spell.event_param3=0 AND spell.event_param4=0 AND spell.event_param5=0
  AND spell.action_type=85 AND spell.action_param1=200832
  AND ((spell.action_param2=0 AND spell.target_type=21) OR (spell.action_param2=128 AND spell.target_type=130))
  AND spell.action_param3=0 AND spell.action_param4=0 AND spell.action_param5=0 AND spell.action_param6=0
  AND spell.target_param1=50 AND spell.target_param2=0 AND spell.target_param3=0 AND spell.target_param4=0
  AND spell.target_x=0 AND spell.target_y=0 AND spell.target_z=0 AND spell.target_o=0;

COMMIT;
