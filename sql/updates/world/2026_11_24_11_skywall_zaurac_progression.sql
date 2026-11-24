-- Zaurac becomes clickable only after his actual surrender advances scenario 983.
START TRANSACTION;

SET @skywall_zaurac_safe :=
    (SELECT COUNT(*) FROM creature_template
     WHERE entry IN (100760,100830) AND AIName = 'SmartAI' AND ScriptName = '' AND (npcflag & 16777216) = 0) = 2
    AND EXISTS (SELECT 1 FROM scenario_data WHERE ScenarioID = 983 AND MapID = 1528 AND Class = 10)
    AND EXISTS (SELECT 1 FROM creature WHERE guid = 372120 AND id = 100760 AND map = 1528)
    AND EXISTS (SELECT 1 FROM npc_spellclick_spells WHERE npc_entry = 100830 AND spell_id = 198749
                AND cast_flags = 1 AND user_type = 0 AND add_npc_flag = 1)
    AND (SELECT COUNT(*) FROM smart_scripts WHERE entryorguid = 100830 AND source_type = 0 AND link IN (2,3,8)) = 3
    AND EXISTS (SELECT 1 FROM smart_scripts WHERE entryorguid = 100830 AND source_type = 0 AND id = 7
                AND link = 8 AND event_type = 2 AND event_param1 = 0 AND event_param2 = 5
                AND action_type = 2 AND action_param1 = 35 AND target_type = 1)
    AND EXISTS (SELECT 1 FROM smart_scripts WHERE entryorguid = 100830 AND source_type = 0 AND id = 8
                AND link = 2 AND event_type = 61 AND action_type = 81 AND action_param1 = 16777216)
    AND EXISTS (SELECT 1 FROM smart_scripts WHERE entryorguid = 100830 AND source_type = 0 AND id = 2
                AND link = 3 AND event_type = 61 AND action_type = 45 AND action_param1 = 6 AND action_param2 = 6
                AND target_type = 19 AND target_param1 = 100762)
    AND EXISTS (SELECT 1 FROM smart_scripts WHERE entryorguid = 100830 AND source_type = 0 AND id = 3
                AND link = 0 AND event_type = 61 AND action_type = 205 AND action_param1 = 92 AND action_param2 = 47952
                AND target_type = 21 AND target_param1 = 50)
    AND EXISTS (SELECT 1 FROM smart_scripts WHERE entryorguid = 100830 AND source_type = 0 AND id = 9
                AND link = 10 AND event_type = 73 AND action_type = 205 AND action_param1 = 92 AND action_param2 = 47975
                AND target_type = 7)
    AND NOT EXISTS (SELECT 1 FROM conditions WHERE SourceTypeOrReferenceId = 18 AND SourceGroup = 100830
                    AND SourceEntry = 198749);

UPDATE creature_template
SET ScriptName = CASE entry WHEN 100760 THEN 'npc_skywall_typhinius' WHEN 100830 THEN 'npc_skywall_zaurac' END
WHERE entry IN (100760,100830) AND @skywall_zaurac_safe;

UPDATE npc_spellclick_spells SET add_npc_flag = 0
WHERE npc_entry = 100830 AND spell_id = 198749 AND @skywall_zaurac_safe;

UPDATE smart_scripts SET link = CASE id WHEN 7 THEN 2 ELSE 0 END
WHERE entryorguid = 100830 AND source_type = 0 AND id IN (2,7) AND @skywall_zaurac_safe;

DELETE FROM smart_scripts
WHERE entryorguid = 100830 AND source_type = 0 AND id IN (3,8) AND @skywall_zaurac_safe;

INSERT INTO conditions
    (SourceTypeOrReferenceId, SourceGroup, SourceEntry, SourceId, ElseGroup, ConditionTypeOrReference,
     ConditionTarget, ConditionValue1, ConditionValue2, ConditionValue3, NegativeCondition, ErrorTextId, ScriptName, Comment)
SELECT 18, 100830, 198749, 0, 0, 9, 0, 40570, 0, 0, 0, 0, '', 'Zaurac - active Into the Heavens' WHERE @skywall_zaurac_safe
UNION ALL
SELECT 18, 100830, 198749, 0, 0, 15, 0, 512, 0, 0, 0, 0, '', 'Zaurac - Monk' WHERE @skywall_zaurac_safe
UNION ALL
SELECT 18, 100830, 198749, 0, 0, 22, 0, 1528, 0, 0, 0, 0, '', 'Zaurac - Skywall' WHERE @skywall_zaurac_safe
UNION ALL
SELECT 18, 100830, 198749, 0, 0, 36, 0, 0, 0, 0, 0, 0, '', 'Zaurac - living player' WHERE @skywall_zaurac_safe
UNION ALL
SELECT 18, 100830, 198749, 0, 0, 44, 0, 983, 4, 0, 0, 0, '', 'Zaurac - Dragon Wrangling' WHERE @skywall_zaurac_safe;

COMMIT;
