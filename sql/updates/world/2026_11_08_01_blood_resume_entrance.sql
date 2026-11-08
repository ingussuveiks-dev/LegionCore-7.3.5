-- Native26972 LFG1059 selects DeathKnight940/map1489/difficulty12.
-- Keep its USER_TELEPORT_NOT_ALLOWED flags. Saved scenario entrances use
-- the actual201678 destination, not a later death position.
START TRANSACTION;
SET @blood_resume_safe :=
    (SELECT COUNT(*) FROM spell_target_position WHERE id=201678)=1
    AND EXISTS (SELECT 1 FROM spell_target_position WHERE id=201678 AND target_map=1489
        AND ABS(target_position_x-240.9)<0.001 AND ABS(target_position_y-1953.44)<0.001
        AND ABS(target_position_z-(-55.29))<0.001 AND ABS(target_orientation-0.384845)<0.001)
    AND (SELECT COUNT(*) FROM scenario_data WHERE ScenarioID=940)=1
    AND EXISTS (SELECT 1 FROM scenario_data WHERE MapID=1489 AND DifficultyID=0 AND Team=0
        AND Class=0 AND LfgDungeonID=0 AND ScenarioID=940)
    AND EXISTS (SELECT 1 FROM smart_scripts WHERE entryorguid=102414 AND source_type=0 AND id=0 AND link=1 AND event_type=10 AND action_type=204 AND action_param1=940 AND target_type=18)
    AND EXISTS (SELECT 1 FROM smart_scripts WHERE entryorguid=102414 AND source_type=0 AND id=1 AND link=0 AND event_type=61 AND action_type=85 AND action_param1=201678 AND target_type=18)
    AND NOT EXISTS (SELECT 1 FROM lfg_entrances WHERE dungeonId IN(1059)
        AND NOT COALESCE((ABS(position_x-240.9)<0.001 AND ABS(position_y-1953.44)<0.001
            AND ABS(position_z-(-55.29))<0.001 AND ABS(orientation-0.384845)<0.001),0));
SET @blood_resume_guard=IF(COALESCE(@blood_resume_safe,0),'DO 0',
    'SIGNAL SQLSTATE ''45000'' SET MESSAGE_TEXT=''Blood resume entrance/source/route differ; review before updating''');
PREPARE blood_resume_statement FROM @blood_resume_guard;
EXECUTE blood_resume_statement;
DEALLOCATE PREPARE blood_resume_statement;
INSERT INTO lfg_entrances (dungeonId,name,position_x,position_y,position_z,orientation)
SELECT native_entrance.dungeonId,'Blood acquisition entrance',
    target_position_x,target_position_y,target_position_z,target_orientation
FROM (SELECT 1059 AS dungeonId) AS native_entrance
CROSS JOIN spell_target_position
WHERE id=201678 AND @blood_resume_safe
    AND EXISTS (SELECT 1 FROM smart_scripts WHERE entryorguid=102414 AND source_type=0 AND id=0 AND link=1 AND event_type=10 AND action_type=204 AND action_param1=940 AND target_type=18)
    AND EXISTS (SELECT 1 FROM smart_scripts WHERE entryorguid=102414 AND source_type=0 AND id=1 AND link=0 AND event_type=61 AND action_type=85 AND action_param1=201678 AND target_type=18)
    AND NOT EXISTS (SELECT 1 FROM lfg_entrances WHERE dungeonId=native_entrance.dungeonId);
COMMIT;
