-- Original phase-5015 Void Barrier segments and invisible collision on map 1494.
START TRANSACTION;
INSERT INTO `gameobject`
    (`guid`,`id`,`map`,`zoneId`,`areaId`,`spawnMask`,`phaseMask`,`PhaseId`,`position_x`,`position_y`,`position_z`,`orientation`,
     `rotation0`,`rotation1`,`rotation2`,`rotation3`,`spawntimesecs`,`animprogress`,`state`)
VALUES
    (3101120,246935,1494,7777,7777,4096,1,'5015',1302.901,-232.1892,39.49883,1.043566,0.0,0.0,0.4984264,0.866932,7200,255,1),
    (3101121,246866,1494,7777,7777,4096,1,'5015',1310.767,-287.2031,45.29362,5.527859,0.0,0.0,-0.3687496,0.9295288,7200,255,1),
    (3101122,246866,1494,7777,7777,4096,1,'5015',1299.024,-229.783,45.42614,4.446345,0.0,0.0,-0.7946434,0.6070765,7200,255,1),
    (3101123,246866,1494,7777,7777,4096,1,'5015',1317.83,-275.4427,44.60416,5.931935,0.0,0.0,-0.1747236,0.9846175,7200,255,1),
    (3101124,246866,1494,7777,7777,4096,1,'5015',1305.481,-232.9167,45.63089,0.9180372,0.0,0.0,0.4430685,0.8964878,7200,255,1),
    (3101125,246935,1494,7777,7777,4096,1,'5015',1317.184,-248.158,39.56079,0.3796506,0.0,0.0,0.1886873,0.9820372,7200,255,1),
    (3101126,246866,1494,7777,7777,4096,1,'5015',1314.556,-242.8906,45.19947,0.523908,0.0,0.0,0.2589684,0.9658858,7200,255,1),
    (3101127,246935,1494,7777,7777,4096,1,'5015',1317.052,-277.0695,39.20011,5.865898,0.0,0.0,-0.2071333,0.9783127,7200,255,1),
    (3101128,246935,1494,7777,7777,4096,1,'5015',1304.262,-292.6875,39.34827,5.267663,0.0,0.0,-0.4862223,0.8738351,7200,255,1),
    (3101129,246935,1494,7777,7777,4096,1,'5015',1311.389,-238.9375,39.48761,0.7384756,0.0,0.0,0.3609047,0.9326027,7200,255,1),
    (3101130,246866,1494,7777,7777,4096,1,'5015',1305.208,-291.9618,45.71334,5.335137,0.0,0.0,-0.4564705,0.8897386,7200,255,1),
    (3101131,246866,1494,7777,7777,4096,1,'5015',1314.762,-282.1528,45.55846,5.770107,0.0,0.0,-0.2537346,0.9672739,7200,255,1),
    (3101132,246866,1494,7777,7777,4096,1,'5015',1310.957,-237.8368,45.50887,0.7606649,0.0,0.0,0.3712292,0.9285413,7200,255,1),
    (3101133,246866,1494,7777,7777,4096,1,'5015',1298.943,-295.6198,45.42615,5.136264,0.0,0.0,-0.5425425,0.8400284,7200,255,1),
    (3101134,246866,1494,7777,7777,4096,1,'5015',1317.734,-249.5608,45.42614,0.3623632,0.0,0.0,0.180192,0.9836314,7200,255,1),
    (3101135,246935,1494,7777,7777,4096,1,'5015',1312.382,-285.0208,39.12182,5.621935,0.0,0.0,-0.3246346,0.9458395,7200,255,1)
ON DUPLICATE KEY UPDATE `id`=`gameobject`.`id`;

-- The actual four creature kills are tracked by native creature criteria.
-- Preserve their spell on death and let the instance acknowledge 48802 only
-- when the player is on the far side of the barrier.
UPDATE `smart_scripts` AS death
JOIN `smart_scripts` AS talk ON talk.`entryorguid` = death.`entryorguid` AND talk.`source_type` = death.`source_type`
    AND talk.`id` = death.`link` AND talk.`id` = 3 AND talk.`link` = 0
SET death.`link` = 0, death.`action_type` = talk.`action_type`,
    death.`action_param1` = talk.`action_param1`, death.`action_param2` = talk.`action_param2`,
    death.`action_param3` = talk.`action_param3`, death.`action_param4` = talk.`action_param4`,
    death.`action_param5` = talk.`action_param5`, death.`action_param6` = talk.`action_param6`,
    death.`comment` = 'Siphoning Rift - On Death - Invoker Cast 203422'
WHERE death.`entryorguid` = 102787 AND death.`source_type` = 0 AND death.`id` = 2 AND death.`event_type` = 6
  AND death.`action_type` = 205 AND death.`action_param1` = 92 AND death.`action_param2` = 48802
  AND death.`target_type` = 7 AND talk.`event_type` = 61 AND talk.`action_type` = 85 AND talk.`action_param1` = 203422
  AND talk.`target_type` = 7;

DELETE talk FROM `smart_scripts` AS talk
JOIN `smart_scripts` AS death ON death.`entryorguid` = talk.`entryorguid` AND death.`source_type` = talk.`source_type`
WHERE talk.`entryorguid` = 102787 AND talk.`source_type` = 0 AND talk.`id` = 3 AND talk.`link` = 0
  AND talk.`event_type` = 61 AND talk.`action_type` = 85 AND talk.`action_param1` = 203422 AND talk.`target_type` = 7
  AND death.`id` = 2 AND death.`link` = 0 AND death.`event_type` = 6
  AND death.`action_type` = 85 AND death.`action_param1` = 203422 AND death.`target_type` = 7;

COMMIT;
