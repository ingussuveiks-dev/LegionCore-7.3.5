-- Both records place the same Mo'arg Painbringer at the same spot in map 1460.
-- Keep the first spawn and the separate patrol near Varian.
DELETE extra
FROM `creature` AS extra
JOIN `creature` AS original ON original.`guid` = 294825
    AND original.`id` = extra.`id` AND original.`map` = extra.`map`
    AND original.`position_x` = extra.`position_x` AND original.`position_y` = extra.`position_y`
    AND original.`position_z` = extra.`position_z` AND original.`orientation` = extra.`orientation`
    AND original.`spawnMask` = extra.`spawnMask` AND original.`phaseMask` = extra.`phaseMask`
    AND original.`PhaseId` = extra.`PhaseId`
WHERE extra.`guid` = 294827 AND extra.`id` = 92564 AND extra.`map` = 1460
    AND extra.`spawnMask` = 4096 AND extra.`phaseMask` = 1 AND extra.`PhaseId` = ''
    AND extra.`MovementType` = 0 AND extra.`spawndist` = 0 AND extra.`spawntimesecs` = 7200
    AND NOT EXISTS (SELECT 1 FROM `creature_addon` WHERE `guid` = extra.`guid`)
    AND NOT EXISTS (SELECT 1 FROM `creature_formations` WHERE `leaderGUID` = extra.`guid` OR `memberGUID` = extra.`guid`)
    AND NOT EXISTS (SELECT 1 FROM `creature_movement_override` WHERE `SpawnId` = extra.`guid`)
    AND NOT EXISTS (SELECT 1 FROM `game_event_creature` WHERE `guid` = extra.`guid`)
    AND NOT EXISTS (SELECT 1 FROM `pool_creature` WHERE `guid` = extra.`guid`)
    AND NOT EXISTS (SELECT 1 FROM `linked_respawn` WHERE `guid` = extra.`guid` OR `linkedGuid` = extra.`guid`)
    AND NOT EXISTS (SELECT 1 FROM `smart_scripts` WHERE `source_type` = 0 AND `entryorguid` = -294827);
