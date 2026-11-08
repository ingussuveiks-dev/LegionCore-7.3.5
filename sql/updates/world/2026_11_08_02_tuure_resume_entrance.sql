-- Native26972 WorldSafeLoc5480: Holy Priest Niskara entrance, facing degrees
-- converted to radians. Do not substitute the Warrior/Hodir harbor portal.
START TRANSACTION;
SET @tuure_resume_safe :=
 (SELECT COUNT(*) FROM scenario_data WHERE ScenarioID=1085)=1
 AND EXISTS (SELECT 1 FROM scenario_data WHERE ScenarioID=1085 AND MapID=1489 AND Class=5 AND Team=0 AND DifficultyID=0 AND LfgDungeonID=0)
 AND EXISTS (SELECT 1 FROM creature WHERE guid=361419 AND id=106134 AND map=1489 AND phaseMask=1 AND phaseId=6374
  AND ABS(position_x-(-23.2934))<0.001 AND ABS(position_y-1170.98)<0.001 AND ABS(position_z-(-69.2009))<0.001)
 AND EXISTS (SELECT 1 FROM creature_template WHERE entry=106134 AND ScriptName='npc_vindicator_boros')
 AND NOT EXISTS (SELECT 1 FROM lfg_entrances WHERE dungeonId=1243 AND NOT COALESCE(
  (ABS(position_x-(-43.1440964))<0.001 AND ABS(position_y-1160.33679)<0.001 AND ABS(position_z-(-74.353569))<0.001 AND ABS(orientation-0.483893174)<0.001)
  OR (name IS NULL AND ABS(position_x-(-828.85))<0.001 AND ABS(position_y-4372)<0.001 AND ABS(position_z-738.64)<0.001 AND ABS(orientation-1.86768)<0.001),0));
SET @tuure_resume_guard=IF(COALESCE(@tuure_resume_safe,0),'DO 0',
 'SIGNAL SQLSTATE ''45000'' SET MESSAGE_TEXT=''Tuure resume entrance/route/initial actor differ; review before updating''');
PREPARE tuure_resume_statement FROM @tuure_resume_guard;
EXECUTE tuure_resume_statement;
DEALLOCATE PREPARE tuure_resume_statement;
UPDATE lfg_entrances SET position_x=-43.1440964,position_y=1160.33679,position_z=-74.353569,orientation=0.483893174
WHERE @tuure_resume_safe AND dungeonId=1243 AND name IS NULL
 AND ABS(position_x-(-828.85))<0.001 AND ABS(position_y-4372)<0.001 AND ABS(position_z-738.64)<0.001 AND ABS(orientation-1.86768)<0.001;
INSERT INTO lfg_entrances (dungeonId,name,position_x,position_y,position_z,orientation)
SELECT 1243,'Tuure acquisition entrance',-43.1440964,1160.33679,-74.353569,0.483893174
WHERE @tuure_resume_safe AND NOT EXISTS (SELECT 1 FROM lfg_entrances WHERE dungeonId=1243);
COMMIT;
