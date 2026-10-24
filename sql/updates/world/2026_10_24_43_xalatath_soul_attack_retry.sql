SET @xalatath_soul_attack_known :=
    (SELECT COUNT(*) FROM creature_template WHERE entry=102247 AND AIName='SmartAI' AND ScriptName='')=1
    AND (SELECT COUNT(*) FROM creature_template WHERE entry=101148 AND AIName='SmartAI' AND ScriptName='npc_xalatath_farthing')=1
    AND (SELECT COUNT(*) FROM smart_scripts WHERE entryorguid=102247)=1
    AND (SELECT COUNT(*) FROM smart_scripts WHERE entryorguid=102247 AND source_type=0 AND id=0 AND link=0
        AND Difficulties='' AND event_type=54 AND event_phase_mask=0 AND event_chance=100 AND event_flags=1
        AND event_param1=0 AND event_param2=0 AND event_param3=0 AND event_param4=0 AND event_param5=0
        AND action_type=11 AND action_param1=201899 AND action_param2=0 AND action_param3=0
        AND action_param4=0 AND action_param5=0 AND action_param6=0
        AND target_type=19 AND target_param1=101148 AND target_param2=30 AND target_param3=0
        AND target_x=0 AND target_y=0 AND target_z=0 AND target_o=0)=1
    AND (SELECT COUNT(*) FROM smart_scripts WHERE entryorguid=101148 AND source_type=0 AND id=29 AND link=0
        AND event_type=8 AND event_phase_mask=0 AND event_chance=100 AND event_flags=1
        AND event_param1=201899 AND event_param2=0 AND action_type=80 AND action_param1=101148
        AND action_param2=2 AND target_type=1)=1
    AND NOT EXISTS (SELECT 1 FROM conditions WHERE SourceTypeOrReferenceId=22
        AND ((SourceEntry=102247 AND SourceId=0) OR (SourceEntry=101148 AND SourceId=0 AND SourceGroup=30)))
    AND NOT EXISTS (SELECT 1 FROM conditions WHERE SourceTypeOrReferenceId=17 AND SourceEntry=201899)
    AND NOT EXISTS (SELECT 1 FROM spell_script_names WHERE spell_id=201899);

UPDATE smart_scripts SET target_type=7,target_param1=0,target_param2=0
WHERE @xalatath_soul_attack_known AND entryorguid=102247 AND source_type=0 AND id=0;

UPDATE creature_template SET ScriptName='npc_xalatath_soul_attack'
WHERE @xalatath_soul_attack_known AND entry=102247;
