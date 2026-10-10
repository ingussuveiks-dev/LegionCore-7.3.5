-- Lifespring: native companion 190370 -> 96038 and reconstructed crystal placements.
-- Coordinates are NOT retail sniff data. See docs/audits/highmountain-lifespring-2026-10-10.md.
UPDATE creature_template SET AIName='', ScriptName='npc_lifespring_companion'
WHERE entry=96038 AND ScriptName IN ('','npc_lifespring_companion');
INSERT INTO spell_script_names (spell_id,ScriptName)
SELECT 190370,'spell_lifespring_companion'
WHERE NOT EXISTS (SELECT 1 FROM spell_script_names WHERE spell_id=190370 AND ScriptName='spell_lifespring_companion');
-- The old end-status mask required High Water to be complete/rewarded before summoning.
-- PlayerScript now covers either parallel quest and cleans up on High Water acceptance.
DELETE FROM spell_area WHERE spell=190370 AND area=7786 AND quest_start=39488 AND quest_end=39498;
INSERT IGNORE INTO creature_queststarter (id,quest) VALUES (96038,39488);

-- Retain the native GO template, model, lock and quest-only loot (one 128393 per node).
-- Let AUTO_INCREMENT allocate GUIDs. Reapplication preserves these and custom nearby spawns.
INSERT INTO gameobject (id,map,zoneId,areaId,spawnMask,phaseMask,PhaseId,
 position_x,position_y,position_z,orientation,rotation0,rotation1,rotation2,rotation3,
 spawntimesecs,animprogress,state)
SELECT 243639,1220,7503,7786,1,1,'',p.x,p.y,p.z,0,0,0,0,1,120,255,1
FROM (
 SELECT 4100.06 AS x,4983.76 AS y,659.759 AS z
 UNION ALL SELECT 4202.04,5020.85,652.417
 UNION ALL SELECT 4177.20,5060.72,660.555
 UNION ALL SELECT 4116.46,5094.84,676.299
 UNION ALL SELECT 4224.64,5049.16,652.758
 UNION ALL SELECT 4193.20,5080.70,661.134
 UNION ALL SELECT 4151.47,5098.82,673.447
 UNION ALL SELECT 4206.49,5070.02,700.487
 UNION ALL SELECT 4211.57,5091.70,669.030
 UNION ALL SELECT 4210.34,5100.36,702.941
 UNION ALL SELECT 4188.76,5119.12,694.154
 UNION ALL SELECT 4138.50,5010.35,648.236
) p WHERE NOT EXISTS (SELECT 1 FROM gameobject g WHERE g.id=243639 AND g.map=1220
 AND ABS(g.position_x-p.x)<1 AND ABS(g.position_y-p.y)<1 AND ABS(g.position_z-p.z)<1);
