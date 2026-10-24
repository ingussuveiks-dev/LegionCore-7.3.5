START TRANSACTION;

SET @talonclaw_harpoon_owner :=
    EXISTS (SELECT 1 FROM creature_template WHERE entry=99431 AND AIName='SmartAI' AND ScriptName='' AND VehicleId=0)
    AND (SELECT COUNT(*) FROM smart_scripts WHERE entryorguid=99431 AND source_type=0)=10
    AND NOT EXISTS (SELECT 1 FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceEntry=99431 AND SourceId=0)
    AND NOT EXISTS (SELECT 1 FROM spell_script_names WHERE spell_id=195475)
    AND EXISTS (SELECT 1 FROM smart_scripts WHERE entryorguid=99431 AND source_type=0 AND id=0 AND link=1 AND event_type=54 AND action_type=11 AND action_param1=195476 AND target_type=1)
    AND EXISTS (SELECT 1 FROM smart_scripts WHERE entryorguid=99431 AND source_type=0 AND id=1 AND link=2 AND event_type=61 AND action_type=11 AND action_param1=182737 AND target_type=7)
    AND EXISTS (SELECT 1 FROM smart_scripts WHERE entryorguid=99431 AND source_type=0 AND id=3 AND link=4 AND event_type=60 AND event_param1=4000 AND event_param2=4000 AND action_type=28 AND action_param1=182737 AND target_type=23)
    AND EXISTS (SELECT 1 FROM smart_scripts WHERE entryorguid=99431 AND source_type=0 AND id=4 AND link=5 AND event_type=61 AND action_type=62 AND action_param1=1511 AND target_type=23)
    AND EXISTS (SELECT 1 FROM smart_scripts WHERE entryorguid=99431 AND source_type=0 AND id=5 AND link=6 AND Difficulties=''
        AND event_type=61 AND event_phase_mask=0 AND event_chance=100 AND event_flags=1
        AND event_param1=0 AND event_param2=0 AND event_param3=0 AND event_param4=0 AND event_param5=0
        AND action_type=205 AND action_param1=92 AND action_param2=47608 AND action_param3=0 AND action_param4=0 AND action_param5=0 AND action_param6=0
        AND ((target_type=21 AND target_param1=100) OR (target_type=23 AND target_param1=0))
        AND target_param2=0 AND target_param3=0 AND target_param4=0 AND target_x=0 AND target_y=0 AND target_z=0 AND target_o=0);

UPDATE smart_scripts SET target_type=23,target_param1=0
WHERE entryorguid=99431 AND source_type=0 AND id=5 AND link=6 AND @talonclaw_harpoon_owner;

COMMIT;
