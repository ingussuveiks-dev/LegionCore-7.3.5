START TRANSACTION;

SET @talonclaw_apata_dialogue :=
    EXISTS (SELECT 1 FROM creature_template WHERE entry=100235 AND AIName='SmartAI' AND ScriptName='')
    AND (SELECT COUNT(*) FROM smart_scripts WHERE entryorguid=100235 AND source_type=0)=23
    AND NOT EXISTS (SELECT 1 FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceEntry=100235 AND SourceId=0 AND SourceGroup IN(16,17,18,19,20))
    AND (SELECT COUNT(*) FROM creature_text WHERE CreatureID=100235 AND GroupID IN(11,12,13,14))=4
    AND (SELECT COUNT(*) FROM creature_text WHERE CreatureID=100235 AND GroupID IN(11,12,13,14) AND ID=0 AND Text<>'' AND Probability=100)=4
    AND (SELECT COUNT(*) FROM waypoints WHERE entry=10023501)=7
    AND (SELECT COUNT(*) FROM smart_scripts WHERE entryorguid=100235 AND source_type=0 AND id BETWEEN 15 AND 19
        AND Difficulties='' AND event_phase_mask=0 AND event_chance=100 AND event_flags=1
        AND event_param3=0 AND event_param4=0 AND event_param5=0
        AND action_param3=0 AND action_param4=0 AND action_param5=0 AND action_param6=0
        AND target_type=1 AND target_param1=0 AND target_param2=0 AND target_param3=0 AND target_param4=0
        AND target_x=0 AND target_y=0 AND target_z=0 AND target_o=0
        AND ((id=15 AND link=0 AND event_type=38 AND event_param1=4 AND event_param2=4 AND action_type=1 AND action_param1=11 AND action_param2=3000)
          OR (id=16 AND link=0 AND event_type=52 AND event_param1=11 AND event_param2=100235 AND action_type=1 AND action_param1=12 AND action_param2=6000)
          OR (id=17 AND link=18 AND event_type=52 AND event_param1=12 AND event_param2=100235 AND action_type=53 AND action_param1=1 AND action_param2=10023501)
          OR (id=18 AND link=0 AND event_type=61 AND event_param1=0 AND event_param2=0 AND action_type IN(1,11) AND action_param1=13 AND action_param2=8000)
          OR (id=19 AND link=0 AND event_type=52 AND event_param1=13 AND event_param2=100235 AND action_type=1 AND action_param1=14 AND action_param2=7000)))=5;

UPDATE smart_scripts SET action_type=1
WHERE entryorguid=100235 AND source_type=0 AND id=18 AND link=0 AND @talonclaw_apata_dialogue;

COMMIT;
