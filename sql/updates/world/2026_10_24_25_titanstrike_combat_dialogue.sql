START TRANSACTION;

SET @titanstrike_combat_dialogue :=
    EXISTS (SELECT 1 FROM creature_template WHERE entry=106714 AND AIName='SmartAI' AND ScriptName='')
    AND (SELECT COUNT(*) FROM smart_scripts WHERE entryorguid=10671400 AND source_type=9)=9
    AND (SELECT COUNT(*) FROM smart_scripts WHERE entryorguid=10671400 AND source_type=9 AND Difficulties=''
        AND event_type=0 AND event_phase_mask=0 AND event_chance=100 AND event_flags=1 AND event_param2=event_param1
        AND event_param3=0 AND event_param4=0 AND event_param5=0 AND link=0 AND action_type=11
        AND action_param2=0 AND action_param3=0 AND action_param4=0 AND action_param5=0 AND action_param6=0
        AND target_type=1 AND target_param1=0 AND target_param2=0 AND target_param3=0 AND target_param4=0
        AND target_x=0 AND target_y=0 AND target_z=0 AND target_o=0
        AND ((id=6 AND event_param1=18000 AND action_param1=211824)
          OR (id=7 AND event_param1=13000 AND action_param1=211826)
          OR (id=8 AND event_param1=20000 AND action_param1=211827)))=3;

INSERT INTO spell_script_names(spell_id,ScriptName)
SELECT spells.spell_id,'spell_titanstrike_combat_dialogue'
FROM (SELECT 211824 AS spell_id UNION ALL SELECT 211826 UNION ALL SELECT 211827) spells
WHERE @titanstrike_combat_dialogue AND NOT EXISTS (
    SELECT 1 FROM spell_script_names existing WHERE existing.spell_id=spells.spell_id
);

COMMIT;
