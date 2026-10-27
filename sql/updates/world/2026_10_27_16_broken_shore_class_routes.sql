-- Map1500 hosts distinct Paladin/Warrior/DemonHunter routes in native26972.
-- Warrior LFG1176 has native ScenarioID0: explicit World routing is necessary.
-- Refuse custom or partially modified routes instead of silently overwriting.
SET @route_safe := (SELECT COUNT(*) FROM scenario_data WHERE MapID=1500 AND ScenarioID IN(775,933,961))=3
 AND (SELECT COUNT(*) FROM scenario_data WHERE MapID=1500 AND
 ((ScenarioID=775 AND ((DifficultyID=0 AND Team=0 AND Class=0 AND LfgDungeonID=0) OR (DifficultyID=12 AND Team=0 AND Class=2 AND LfgDungeonID=906)))
 OR (ScenarioID=933 AND ((DifficultyID=0 AND Team=0 AND Class=0 AND LfgDungeonID=0) OR (DifficultyID=12 AND Team=0 AND Class=1 AND LfgDungeonID=1176)))
 OR (ScenarioID=961 AND ((DifficultyID=0 AND Team=0 AND Class=0 AND LfgDungeonID=0) OR (DifficultyID=12 AND Team=0 AND Class=12 AND LfgDungeonID=1166)))))=3;
SET @route_check := IF(@route_safe,'DO 0','SIGNAL SQLSTATE ''45000'' SET MESSAGE_TEXT=''Map1500 class routes differ; review before updating''');
PREPARE route_guard FROM @route_check; EXECUTE route_guard; DEALLOCATE PREPARE route_guard;
UPDATE scenario_data SET DifficultyID=12,Team=0,Class=CASE ScenarioID WHEN 775 THEN 2 WHEN 933 THEN 1 WHEN 961 THEN 12 END,
 LfgDungeonID=CASE ScenarioID WHEN 775 THEN 906 WHEN 933 THEN 1176 WHEN 961 THEN 1166 END
 WHERE MapID=1500 AND ScenarioID IN(775,933,961);
