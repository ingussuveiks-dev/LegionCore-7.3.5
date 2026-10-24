START TRANSACTION;

SET @titanstrike_warning :=
    EXISTS (SELECT 1 FROM creature_template WHERE entry=106730 AND AIName='SmartAI' AND ScriptName='npc_titanstrike_final_prustaga')
    AND (SELECT COUNT(*) FROM smart_scripts WHERE entryorguid=106730 AND source_type=9)=7
    AND EXISTS (SELECT 1 FROM smart_scripts WHERE entryorguid=106730 AND source_type=9 AND id=0 AND action_type=11 AND action_param1=211753 AND target_type=1)
    AND EXISTS (SELECT 1 FROM smart_scripts WHERE entryorguid=106730 AND source_type=9 AND id=1 AND action_type=205 AND action_param1=92 AND action_param2=50921)
    AND EXISTS (SELECT 1 FROM smart_scripts WHERE entryorguid=106730 AND source_type=9 AND id=2 AND link=0 AND Difficulties=''
        AND event_type=0 AND event_phase_mask=0 AND event_chance=100 AND event_flags=1
        AND event_param1 IN(5000,15700) AND event_param2=event_param1 AND event_param3=0 AND event_param4=0 AND event_param5=0
        AND action_type=11 AND action_param1=208447 AND action_param2=0 AND action_param3=0 AND action_param4=0 AND action_param5=0 AND action_param6=0
        AND target_type=1 AND target_param1=0 AND target_param2=0 AND target_param3=0 AND target_param4=0
        AND target_x=0 AND target_y=0 AND target_z=0 AND target_o=0)
    AND EXISTS (SELECT 1 FROM smart_scripts WHERE entryorguid=106730 AND source_type=9 AND id=3 AND event_param1=0 AND event_param2=0 AND action_type=11 AND action_param1=211848 AND target_type=1)
    AND EXISTS (SELECT 1 FROM smart_scripts WHERE entryorguid=106730 AND source_type=9 AND id=4 AND event_param1=5000 AND event_param2=5000 AND action_type=75 AND action_param1=211934 AND target_type=21 AND target_param1=50)
    AND EXISTS (SELECT 1 FROM smart_scripts WHERE entryorguid=106730 AND source_type=9 AND id=5 AND action_type=45 AND action_param1=1 AND action_param2=1 AND target_type=19 AND target_param1=103154)
    AND EXISTS (SELECT 1 FROM smart_scripts WHERE entryorguid=106730 AND source_type=9 AND id=7 AND action_type=37 AND target_type=1)
    AND NOT EXISTS (SELECT 1 FROM smart_scripts WHERE entryorguid=106730 AND source_type=9 AND id=6)
    AND EXISTS (SELECT 1 FROM spell_script_names WHERE spell_id=211960 AND ScriptName='spell_titanstrike_aftermath')
    AND NOT EXISTS (SELECT 1 FROM spell_script_names WHERE spell_id=211753)
    AND (SELECT COUNT(*) FROM conversation_actor WHERE entry=2147)=3
    AND (SELECT COUNT(*) FROM conversation_actor WHERE entry=2147 AND unk1=0 AND unk2=1 AND unk3=0
        AND ((id=0 AND actorId=52951 AND creatureId=106748 AND displayId=69706 AND duration=15000)
          OR (id=1 AND actorId=52944 AND creatureId=106714 AND displayId=69753 AND duration=3550)
          OR (id=2 AND actorId=52946 AND creatureId=106715 AND displayId=66838 AND duration=3550)))=3
    AND NOT EXISTS (SELECT 1 FROM conversation_creature WHERE entry=2147)
    AND (SELECT COUNT(*) FROM conversation_data WHERE entry=2147)=3
    AND (SELECT COUNT(*) FROM conversation_data WHERE entry=2147
        AND ((id=4555 AND idx=0 AND textId=0 AND unk1=137 AND unk2=0)
          OR (id=4556 AND idx=1 AND textId=7800 AND unk1=599 AND unk2=1)
          OR (id=4557 AND idx=2 AND textId=15700 AND unk1=90 AND unk2=2)))=3;

UPDATE smart_scripts SET
    event_param1=(SELECT textId FROM conversation_data WHERE entry=2147 AND id=4557 AND idx=2),
    event_param2=(SELECT textId FROM conversation_data WHERE entry=2147 AND id=4557 AND idx=2)
WHERE entryorguid=106730 AND source_type=9 AND id=2 AND @titanstrike_warning;

COMMIT;
