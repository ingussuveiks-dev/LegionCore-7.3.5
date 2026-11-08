-- Native207683 is the actual acquisition travel source. WorldSafeLoc5443 is
-- the separate lower graveyard; retain the spell destination and facing here.
START TRANSACTION;
SET @sharasdal_resume_safe :=
 (SELECT COUNT(*) FROM scenario_data WHERE ScenarioID=1066)=1
 AND EXISTS (SELECT 1 FROM scenario_data WHERE ScenarioID=1066 AND MapID=1600 AND Class=7 AND Team=0 AND DifficultyID=0 AND LfgDungeonID=0)
 AND EXISTS (SELECT 1 FROM spell_target_position WHERE id=207683 AND target_map=1600
  AND ABS(target_position_x-(-629.42))<0.001 AND ABS(target_position_y-808.68)<0.001
  AND ABS(target_position_z-247.67)<0.001 AND ABS(target_orientation-6.2797)<0.001)
 AND EXISTS (SELECT 1 FROM creature WHERE guid=362063 AND id=102826 AND map=1600 AND phaseMask=1 AND phaseId='6120'
  AND ABS(position_x-(-619.811))<0.001 AND ABS(position_y-804.37)<0.001 AND ABS(position_z-245.375)<0.001)
 AND EXISTS (SELECT 1 FROM creature_template WHERE entry=102826 AND ScriptName='npc_erunak_stonespeaker')
 AND (SELECT COUNT(*) FROM quest_objectives WHERE QuestID=40341 AND Type=0 AND Amount=1 AND ObjectID IN(105805,105787,105806,111286))=4
 AND NOT EXISTS (SELECT 1 FROM lfg_entrances WHERE dungeonId=1229 AND NOT COALESCE(
  (ABS(position_x-(-629.42))<0.001 AND ABS(position_y-808.68)<0.001 AND ABS(position_z-247.67)<0.001 AND ABS(orientation-6.2797)<0.001)
  OR (name IS NULL AND ABS(position_x-836.16)<0.001 AND ABS(position_y-1035.4)<0.001 AND ABS(position_z-48.61)<0.001 AND ABS(orientation-1.7935)<0.001),0));
SET @sharasdal_resume_guard=IF(COALESCE(@sharasdal_resume_safe,0),'DO 0',
 'SIGNAL SQLSTATE ''45000'' SET MESSAGE_TEXT=''Sharasdal resume entrance/portal/route/initial actor differ; review before updating''');
PREPARE sharasdal_resume_statement FROM @sharasdal_resume_guard;
EXECUTE sharasdal_resume_statement;
DEALLOCATE PREPARE sharasdal_resume_statement;
UPDATE lfg_entrances SET position_x=-629.42,position_y=808.68,position_z=247.67,orientation=6.2797
WHERE @sharasdal_resume_safe AND dungeonId=1229 AND name IS NULL
 AND ABS(position_x-836.16)<0.001 AND ABS(position_y-1035.4)<0.001 AND ABS(position_z-48.61)<0.001 AND ABS(orientation-1.7935)<0.001;
INSERT INTO lfg_entrances (dungeonId,name,position_x,position_y,position_z,orientation)
SELECT 1229,'Sharasdal acquisition entrance',-629.42,808.68,247.67,6.2797
WHERE @sharasdal_resume_safe AND NOT EXISTS (SELECT 1 FROM lfg_entrances WHERE dungeonId=1229);
COMMIT;
