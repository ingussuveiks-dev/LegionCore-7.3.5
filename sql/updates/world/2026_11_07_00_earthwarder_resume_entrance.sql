-- Native 26972 LFG1028 is Warrior909/map1495/difficulty12. Its user LFG
-- teleport is disabled, but recorded scenario-login recovery needs an entrance.
-- Use the destination of187803 "Jump to Prot Scenario", not the shared
-- Hunter/Paladin entrances and not a later chamber or saved combat position.
START TRANSACTION;
SET @earthwarder_resume_safe :=
    (SELECT COUNT(*) FROM spell_target_position WHERE id=187803)=1
    AND EXISTS (SELECT 1 FROM spell_target_position WHERE id=187803 AND target_map=1495
        AND ABS(target_position_x-4916.3)<0.001 AND ABS(target_position_y+94)<0.001
        AND ABS(target_position_z-66.3)<0.001 AND ABS(target_orientation-2.45)<0.001)
    AND (SELECT COUNT(*) FROM scenario_data WHERE ScenarioID=909)=1
    AND EXISTS (SELECT 1 FROM scenario_data WHERE MapID=1495 AND DifficultyID=0 AND Team=0
        AND Class=1 AND LfgDungeonID=0 AND ScenarioID=909)
    AND ((SELECT COUNT(*) FROM lfg_entrances WHERE dungeonId=1028)=0
        OR EXISTS (SELECT 1 FROM lfg_entrances WHERE dungeonId=1028
            AND ABS(position_x-4916.3)<0.001 AND ABS(position_y+94)<0.001
            AND ABS(position_z-66.3)<0.001 AND ABS(orientation-2.45)<0.001));
SET @earthwarder_resume_guard=IF(COALESCE(@earthwarder_resume_safe,0),'DO 0',
    'SIGNAL SQLSTATE ''45000'' SET MESSAGE_TEXT=''Earthwarder resume entrance/source/route differ; review before updating''');
PREPARE earthwarder_resume_statement FROM @earthwarder_resume_guard;
EXECUTE earthwarder_resume_statement;
DEALLOCATE PREPARE earthwarder_resume_statement;
INSERT INTO lfg_entrances (dungeonId,name,position_x,position_y,position_z,orientation)
SELECT 1028,'Scale of the Earth-Warder',target_position_x,target_position_y,target_position_z,target_orientation
FROM spell_target_position WHERE id=187803 AND @earthwarder_resume_safe
    AND NOT EXISTS (SELECT 1 FROM lfg_entrances WHERE dungeonId=1028);
COMMIT;
