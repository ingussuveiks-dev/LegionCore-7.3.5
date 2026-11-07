-- Native26972 LFG1162 selects Hunter972/map1489/difficulty12.
-- Keep its USER_TELEPORT_NOT_ALLOWED flags. Saved scenario entrances use
-- the actual201674 destination, not a later death position.
START TRANSACTION;
SET @marksmanship_resume_safe :=
    (SELECT COUNT(*) FROM spell_target_position WHERE id=201674)=1
    AND EXISTS (SELECT 1 FROM spell_target_position WHERE id=201674 AND target_map=1489
        AND ABS(target_position_x-234.68)<0.001 AND ABS(target_position_y-1950.91)<0.001
        AND ABS(target_position_z-(-55.89))<0.001 AND ABS(target_orientation-0.282743)<0.001)
    AND (SELECT COUNT(*) FROM scenario_data WHERE ScenarioID=972)=1
    AND EXISTS (SELECT 1 FROM scenario_data WHERE MapID=1489 AND DifficultyID=0 AND Team=0
        AND Class=3 AND LfgDungeonID=0 AND ScenarioID=972)
    AND NOT EXISTS (SELECT 1 FROM lfg_entrances WHERE dungeonId IN(1162)
        AND NOT COALESCE((ABS(position_x-234.68)<0.001 AND ABS(position_y-1950.91)<0.001
            AND ABS(position_z-(-55.89))<0.001 AND ABS(orientation-0.282743)<0.001),0));
SET @marksmanship_resume_guard=IF(COALESCE(@marksmanship_resume_safe,0),'DO 0',
    'SIGNAL SQLSTATE ''45000'' SET MESSAGE_TEXT=''Marksmanship resume entrance/source/route differ; review before updating''');
PREPARE marksmanship_resume_statement FROM @marksmanship_resume_guard;
EXECUTE marksmanship_resume_statement;
DEALLOCATE PREPARE marksmanship_resume_statement;
INSERT INTO lfg_entrances (dungeonId,name,position_x,position_y,position_z,orientation)
SELECT native_entrance.dungeonId,'Marksmanship acquisition entrance',
    target_position_x,target_position_y,target_position_z,target_orientation
FROM (SELECT 1162 AS dungeonId) AS native_entrance
CROSS JOIN spell_target_position
WHERE id=201674 AND @marksmanship_resume_safe
    AND NOT EXISTS (SELECT 1 FROM lfg_entrances WHERE dungeonId=native_entrance.dungeonId);
COMMIT;
