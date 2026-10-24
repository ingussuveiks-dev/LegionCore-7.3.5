START TRANSACTION;

SET @titanstrike_handoff :=
    EXISTS (SELECT 1 FROM creature_template WHERE entry=106730 AND AIName='SmartAI' AND ScriptName='npc_titanstrike_final_prustaga')
    AND EXISTS (SELECT 1 FROM creature_template WHERE entry=103154 AND AIName='SmartAI' AND ScriptName='')
    AND EXISTS (SELECT 1 FROM spell_linked_spell WHERE spell_trigger=-211934 AND spell_effect=211960
        AND type=0 AND caster=0 AND target=0 AND actiontype=0 AND chance=0 AND cooldown=0
        AND hastalent=0 AND hastalent2=0 AND removeMask=0 AND effectMask=0)
    AND (SELECT COUNT(*) FROM conversation_actor WHERE entry=2167)=3
    AND (SELECT COUNT(*) FROM conversation_actor WHERE entry=2167 AND unk1=0 AND unk2=1 AND unk3=0
         AND ((id=0 AND actorId=52946 AND creatureId=106715 AND displayId=66838 AND duration=50000)
           OR (id=1 AND actorId=52943 AND creatureId=106713 AND displayId=28578 AND duration=9400)
           OR (id=2 AND actorId=52944 AND creatureId=106714 AND displayId=69753 AND duration=9400)))=3
    AND NOT EXISTS (SELECT 1 FROM conversation_creature WHERE entry=2167)
    AND (SELECT COUNT(*) FROM conversation_data WHERE entry=2167)=5
    AND (SELECT COUNT(*) FROM conversation_data WHERE entry=2167
         AND ((id=4598 AND idx=0 AND textId=0 AND unk1=91 AND unk2=0)
           OR (id=4599 AND idx=1 AND textId=6600 AND unk1=621 AND unk2=1)
           OR (id=4600 AND idx=2 AND textId=16900 AND unk1=621 AND unk2=1)
           OR (id=4601 AND idx=3 AND textId=31550 AND unk1=598 AND unk2=2)
           OR (id=4826 AND idx=4 AND textId=44200 AND unk1=621 AND unk2=1)))=5
    AND (SELECT COUNT(*) FROM smart_scripts WHERE entryorguid=103154 AND source_type=0)=7
    AND EXISTS (SELECT 1 FROM smart_scripts WHERE entryorguid=103154 AND source_type=0 AND id=0 AND link=1 AND event_type=38 AND event_param1=1 AND event_param2=1 AND event_flags=1 AND action_type=128 AND action_param1=8631)
    AND EXISTS (SELECT 1 FROM smart_scripts WHERE entryorguid=103154 AND source_type=0 AND id=1 AND link=3 AND event_type=61 AND action_type=11 AND action_param1=211970 AND target_type=1)
    AND EXISTS (SELECT 1 FROM smart_scripts WHERE entryorguid=103154 AND source_type=0 AND id=3 AND link=0 AND Difficulties=''
        AND event_type=61 AND event_phase_mask=0 AND event_chance=100 AND event_flags=1
        AND event_param1=0 AND event_param2=0 AND event_param3=0 AND event_param4=0 AND event_param5=0
        AND action_param1=16777216 AND action_param2=0 AND action_param3=0 AND action_param4=0 AND action_param5=0 AND action_param6=0
        AND target_type=1 AND target_param1=0 AND target_param2=0 AND target_param3=0 AND target_param4=0
        AND target_x=0 AND target_y=0 AND target_z=0 AND target_o=0)
    AND EXISTS (SELECT 1 FROM smart_scripts WHERE entryorguid=106730 AND source_type=9 AND id=0 AND action_type=11 AND action_param1=211753 AND target_type=1)
    AND EXISTS (SELECT 1 FROM smart_scripts WHERE entryorguid=106730 AND source_type=9 AND id=4 AND event_param1=5000 AND event_param2=5000 AND action_type=75 AND action_param1=211934 AND target_type=21 AND target_param1=50)
    AND EXISTS (SELECT 1 FROM smart_scripts WHERE entryorguid=106730 AND source_type=9 AND id=5 AND action_type=45 AND action_param1=1 AND action_param2=1 AND target_type=19 AND target_param1=103154)
    AND EXISTS (SELECT 1 FROM smart_scripts WHERE entryorguid=106730 AND source_type=9 AND id=7 AND action_type=37 AND target_type=1)
    AND NOT EXISTS (SELECT 1 FROM spell_script_names WHERE spell_id=211960 AND ScriptName<>'spell_titanstrike_aftermath')
    AND (
        ((SELECT COUNT(*) FROM smart_scripts WHERE entryorguid=106730 AND source_type=9)=8
         AND EXISTS (SELECT 1 FROM smart_scripts WHERE entryorguid=106730 AND source_type=9 AND id=6 AND link=0 AND Difficulties=''
             AND event_type=0 AND event_phase_mask=0 AND event_chance=100 AND event_flags=1
             AND event_param1=0 AND event_param2=0 AND event_param3=0 AND event_param4=0 AND event_param5=0
             AND action_type=205 AND action_param1=92 AND action_param2=50997 AND action_param3=0 AND action_param4=0 AND action_param5=0 AND action_param6=0
             AND target_type=21 AND target_param1=100 AND target_param2=0 AND target_param3=0 AND target_param4=0
             AND target_x=0 AND target_y=0 AND target_z=0 AND target_o=0)
         AND EXISTS (SELECT 1 FROM smart_scripts WHERE entryorguid=103154 AND source_type=0 AND id=3 AND action_type=81)
         AND NOT EXISTS (SELECT 1 FROM spell_script_names WHERE spell_id=211960))
        OR
        ((SELECT COUNT(*) FROM smart_scripts WHERE entryorguid=106730 AND source_type=9)=7
         AND NOT EXISTS (SELECT 1 FROM smart_scripts WHERE entryorguid=106730 AND source_type=9 AND id=6)
         AND EXISTS (SELECT 1 FROM smart_scripts WHERE entryorguid=103154 AND source_type=0 AND id=3 AND action_type=83)
         AND EXISTS (SELECT 1 FROM spell_script_names WHERE spell_id=211960 AND ScriptName='spell_titanstrike_aftermath'))
    );

UPDATE smart_scripts SET action_type=83
WHERE entryorguid=103154 AND source_type=0 AND id=3 AND @titanstrike_handoff;

DELETE FROM smart_scripts
WHERE entryorguid=106730 AND source_type=9 AND id=6 AND @titanstrike_handoff;

INSERT INTO spell_script_names(spell_id,ScriptName)
SELECT 211960,'spell_titanstrike_aftermath'
WHERE @titanstrike_handoff AND NOT EXISTS (SELECT 1 FROM spell_script_names WHERE spell_id=211960);

COMMIT;
