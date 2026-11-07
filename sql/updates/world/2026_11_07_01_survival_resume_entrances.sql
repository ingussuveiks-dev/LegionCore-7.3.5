-- Native26972 LFG1178/1384 both select Hunter973/map1511/difficulty12.
-- Keep their USER_TELEPORT_NOT_ALLOWED flags. Saved scenario entrances use
-- the actual198950 Fly to Stormheim destination, not a later death position.
START TRANSACTION;
SET @survival_resume_safe :=
    (SELECT COUNT(*) FROM spell_target_position WHERE id=198950)=1
    AND EXISTS (SELECT 1 FROM spell_target_position WHERE id=198950 AND target_map=1511
        AND ABS(target_position_x-3364.77)<0.001 AND ABS(target_position_y-1919.54)<0.001
        AND ABS(target_position_z-0.092994)<0.001 AND ABS(target_orientation-5.69981)<0.001)
    AND (SELECT COUNT(*) FROM scenario_data WHERE ScenarioID=973)=1
    AND EXISTS (SELECT 1 FROM scenario_data WHERE MapID=1511 AND DifficultyID=0 AND Team=0
        AND Class=3 AND LfgDungeonID=0 AND ScenarioID=973)
    AND NOT EXISTS (SELECT 1 FROM lfg_entrances WHERE dungeonId IN(1178,1384)
        AND NOT COALESCE((ABS(position_x-3364.77)<0.001 AND ABS(position_y-1919.54)<0.001
            AND ABS(position_z-0.092994)<0.001 AND ABS(orientation-5.69981)<0.001),0));
SET @survival_resume_guard=IF(COALESCE(@survival_resume_safe,0),'DO 0',
    'SIGNAL SQLSTATE ''45000'' SET MESSAGE_TEXT=''Survival resume entrance/source/route differ; review before updating''');
PREPARE survival_resume_statement FROM @survival_resume_guard;
EXECUTE survival_resume_statement;
DEALLOCATE PREPARE survival_resume_statement;
INSERT INTO lfg_entrances (dungeonId,name,position_x,position_y,position_z,orientation)
SELECT native_entrance.dungeonId,'Survival acquisition entrance',
    target_position_x,target_position_y,target_position_z,target_orientation
FROM (SELECT 1178 AS dungeonId UNION ALL SELECT 1384) AS native_entrance
CROSS JOIN spell_target_position
WHERE id=198950 AND @survival_resume_safe
    AND NOT EXISTS (SELECT 1 FROM lfg_entrances WHERE dungeonId=native_entrance.dungeonId);
COMMIT;
