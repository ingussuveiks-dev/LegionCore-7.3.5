START TRANSACTION;

SET @marksman_farewell_budget :=
    (SELECT COUNT(*) FROM conversation_creature WHERE entry=1072)=3
    AND EXISTS (SELECT 1 FROM conversation_creature WHERE entry=1072 AND id=0 AND creatureId=100397 AND duration IN(25000,40000))
    AND EXISTS (SELECT 1 FROM conversation_creature WHERE entry=1072 AND id=1 AND creatureId=100398 AND duration=2650)
    AND EXISTS (SELECT 1 FROM conversation_creature WHERE entry=1072 AND id=2 AND creatureId=0 AND duration=2650)
    AND NOT EXISTS (SELECT 1 FROM conversation_actor WHERE entry=1072)
    AND (SELECT COUNT(*) FROM conversation_data WHERE entry=1072)=6
    AND (SELECT COUNT(*) FROM conversation_data WHERE entry=1072 AND unk1=0
         AND ((id=2625 AND idx=0 AND textId=0 AND unk2=0)
           OR (id=2630 AND idx=1 AND textId=4550 AND unk2=1)
           OR (id=2658 AND idx=2 AND textId=9850 AND unk2=0)
           OR (id=2659 AND idx=3 AND textId=13050 AND unk2=1)
           OR (id=3427 AND idx=4 AND textId=18050 AND unk2=258)
           OR (id=2631 AND idx=5 AND textId=23050 AND unk2=0)))=6
    AND EXISTS (SELECT 1 FROM creature_template WHERE entry=100398 AND AIName='SmartAI' AND ScriptName='')
    AND (SELECT COUNT(*) FROM smart_scripts WHERE entryorguid=100398 AND source_type=9)=1
    AND EXISTS (SELECT 1 FROM smart_scripts WHERE entryorguid=100398 AND source_type=9 AND id=0 AND link=0
        AND event_type=0 AND event_phase_mask=0 AND event_chance=100 AND event_flags=1 AND Difficulties=''
        AND event_param1 IN(10000,50500) AND event_param2=event_param1
        AND event_param3=0 AND event_param4=0 AND event_param5=0
        AND action_type=22 AND action_param1=1 AND action_param2=0 AND action_param3=0
        AND action_param4=0 AND action_param5=0 AND action_param6=0
        AND target_type=1 AND target_param1=0 AND target_param2=0 AND target_param3=0 AND target_param4=0
        AND target_x=0 AND target_y=0 AND target_z=0 AND target_o=0)
    AND EXISTS (SELECT 1 FROM smart_scripts WHERE entryorguid=100398 AND source_type=0 AND id=5 AND link=6
        AND event_type=10 AND event_phase_mask=1 AND event_chance=100 AND event_flags=1
        AND event_param1=1 AND event_param2=50 AND event_param3=0 AND event_param4=0 AND event_param5=0
        AND action_type=205 AND action_param1=92 AND action_param2=48071 AND action_param3=0
        AND action_param4=0 AND action_param5=0 AND action_param6=0
        AND target_type=7 AND target_param1=0 AND target_param2=0 AND target_param3=0 AND target_param4=0)
    AND EXISTS (SELECT 1 FROM smart_scripts WHERE entryorguid=100398 AND source_type=0 AND id=6 AND link=0
        AND event_type=61 AND action_type=37 AND target_type=1)
    AND NOT EXISTS (SELECT 1 FROM smart_scripts WHERE entryorguid=100398 AND source_type=0
        AND ((event_phase_mask<>0 AND id<>5) OR action_type IN(22,23,30,31)));

UPDATE conversation_creature SET duration=40000
WHERE entry=1072 AND id=0 AND @marksman_farewell_budget;

UPDATE smart_scripts SET event_param1=50500,event_param2=50500
WHERE entryorguid=100398 AND source_type=9 AND id=0 AND @marksman_farewell_budget;

COMMIT;
