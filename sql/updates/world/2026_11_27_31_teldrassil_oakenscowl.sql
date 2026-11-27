-- The Cataclysm spawn matches the Legion quest's northeast Wellspring objective.
INSERT INTO `creature`
    (`guid`,`id`,`map`,`zoneId`,`areaId`,`spawnMask`,`phaseMask`,
     `position_x`,`position_y`,`position_z`,`orientation`,`spawntimesecs`,`spawndist`,`MovementType`)
SELECT 3101073,2166,1,141,141,1,1,10469.4,1451.67,1327.55,0.128997,300,3,1
WHERE NOT EXISTS (SELECT 1 FROM `creature` WHERE `id`=2166 OR `guid`=3101073);

-- Remove the quest-only tumor when abandoning the quest.
UPDATE `quest_template` SET `ItemDrop1`=8136,`ItemDropQuantity1`=1
WHERE `ID`=2499 AND `ItemDrop1`=0 AND `ItemDropQuantity1`=0
  AND 8136 NOT IN (`ItemDrop2`,`ItemDrop3`,`ItemDrop4`);
