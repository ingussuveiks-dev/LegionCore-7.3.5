-- EventObject 101 finishes at the native Raging Winds Bypassed criterion.
-- Scion death opens the route and the next scenario step starts its own waves.
START TRANSACTION;

SET @skywall_winds_safe :=
    (SELECT COUNT(*) FROM eventobject WHERE id = 101) = 1
    AND EXISTS (SELECT 1 FROM eventobject
                WHERE guid = 400104 AND id = 101 AND map = 1528 AND spawnMask = 4096)
    AND EXISTS (SELECT 1 FROM eventobject_template
                WHERE entry = 101 AND ScriptName = 'SmartEventObject' AND radius = 5
                  AND SpellID = 0 AND WorldSafeLocID = 0 AND Flags = 0)
    AND EXISTS (SELECT 1 FROM scenario_data WHERE MapID = 1528 AND ScenarioID = 983 AND Class = 10)
    AND (SELECT COUNT(*) FROM smart_scripts WHERE source_type = 13 AND entryorguid IN (101,-400104)) = 1
    AND EXISTS (
        SELECT 1 FROM smart_scripts
        WHERE entryorguid = 101 AND source_type = 13 AND id = 0 AND link IN (0,1)
          AND Difficulties = '' AND event_type = 89 AND event_phase_mask = 0 AND event_chance = 100
          AND event_flags = 0 AND event_param1 = 0 AND event_param2 = 0 AND event_param3 = 0
          AND event_param4 = 0 AND event_param5 = 0
          AND action_type = 205 AND action_param1 = 92 AND action_param2 = 47962
          AND action_param3 = 0 AND action_param4 = 0 AND action_param5 = 0 AND action_param6 = 0
          AND target_type = 7 AND target_param1 = 0 AND target_param2 = 0 AND target_param3 = 0
          AND target_param4 = 0 AND target_x = 0 AND target_y = 0 AND target_z = 0 AND target_o = 0
    )
    AND EXISTS (SELECT 1 FROM smart_scripts
                WHERE source_type = 0 AND entryorguid = 100647 AND id = 2 AND event_type = 6
                  AND action_type = 45 AND action_param1 = 4 AND action_param2 = 4
                  AND target_type = 19 AND target_param1 = 100762)
    AND EXISTS (SELECT 1 FROM smart_scripts
                WHERE source_type = 0 AND entryorguid = 100762 AND id = 9 AND event_type = 38
                  AND event_param1 = 4 AND event_param2 = 4 AND link = 10)
    AND EXISTS (SELECT 1 FROM smart_scripts
                WHERE source_type = 0 AND entryorguid = 100762 AND id = 10 AND event_type = 61
                  AND action_type = 9 AND target_type = 15 AND target_param1 = 246378)
    AND EXISTS (SELECT 1 FROM smart_scripts
                WHERE source_type = 0 AND entryorguid = 100762 AND id = 12 AND event_type = 60
                  AND event_flags = 1 AND action_type = 80 AND action_param1 = 100762)
    AND EXISTS (SELECT 1 FROM conditions
                WHERE SourceTypeOrReferenceId = 22 AND SourceGroup = 13 AND SourceEntry = 100762
                  AND SourceId = 0 AND ElseGroup = 0 AND ConditionTypeOrReference = 44
                  AND ConditionTarget = 1 AND ConditionValue1 = 983 AND ConditionValue2 = 3
                  AND NegativeCondition = 0);

SET @skywall_winds_guard := IF(@skywall_winds_safe, 'DO 0',
    'SIGNAL SQLSTATE ''45000'' SET MESSAGE_TEXT = ''Skywall winds event 101 or its downstream scenario differs''');
PREPARE skywall_winds_check FROM @skywall_winds_guard;
EXECUTE skywall_winds_check;
DEALLOCATE PREPARE skywall_winds_check;

UPDATE smart_scripts SET link = 0
WHERE entryorguid = 101 AND source_type = 13 AND id = 0 AND link = 1 AND @skywall_winds_safe;

COMMIT;
