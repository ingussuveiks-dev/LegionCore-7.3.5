-- Use the Legion barrier model at the surveyed crest of the central stair.
-- The visual sits low at the stair edge. The invisible doors provide collision.
INSERT INTO `gameobject`
    (`guid`,`id`,`map`,`zoneId`,`areaId`,`spawnMask`,`phaseMask`,`PhaseId`,`position_x`,`position_y`,`position_z`,`orientation`,
     `rotation0`,`rotation1`,`rotation2`,`rotation3`,`spawntimesecs`,`animprogress`,`state`)
VALUES
    (3101136,246866,1494,7777,7777,4096,1,'5015',1318.88,-269.25,40.16,6.2431853,0,0,-0.0199986667,0.9998000067,7200,255,1),
    (3101137,246866,1494,7777,7777,4096,1,'5015',1318.99,-262.49,40.16,0,0,0,0,1,7200,255,1),
    (3101138,246866,1494,7777,7777,4096,1,'5015',1319.32,-255.72,40.16,0.06,0,0,0.0299955002,0.9995500337,7200,255,1),
    (3101139,246935,1494,7777,7777,4096,1,'5015',1319.09,-267.82,39.43,6.2631853,0,0,-0.0099998333,0.9999500004,7200,255,1),
    (3101140,246935,1494,7777,7777,4096,1,'5015',1319.17,-257.19,39.43,0.02,0,0,0.0099998333,0.9999500004,7200,255,1)
ON DUPLICATE KEY UPDATE `id` = `gameobject`.`id`;
