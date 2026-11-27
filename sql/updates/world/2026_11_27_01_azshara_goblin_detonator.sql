-- The archived spawn coordinates place the actual button on Box C beside Gorek.
INSERT INTO `gameobject`
    (`guid`,`id`,`map`,`zoneId`,`areaId`,`spawnMask`,`phaseMask`,`position_x`,`position_y`,`position_z`,`orientation`,
     `rotation0`,`rotation1`,`rotation2`,`rotation3`,`spawntimesecs`,`animprogress`,`state`)
VALUES
    (181401,196870,1,16,3137,1,1,2596.46,-4167.90,159.277,1.90241,0,0,0.814116,0.580703,300,255,1),
    (181402,184856,1,16,3137,1,1,2596.62,-4168.42,158.645,1.91986,0,0,0.819152,0.573577,300,255,1)
ON DUPLICATE KEY UPDATE `id`=`gameobject`.`id`;

INSERT INTO `creature`
    (`guid`,`id`,`map`,`zoneId`,`areaId`,`spawnMask`,`phaseMask`,`modelid`,`equipment_id`,
     `position_x`,`position_y`,`position_z`,`orientation`,`spawntimesecs`,`curhealth`)
VALUES
    (3101072,36662,1,16,3137,1,1,30280,1,2595.40,-4170.65,158.962,1.11701,300,539)
ON DUPLICATE KEY UPDATE `id`=`creature`.`id`;

-- Use the existing localized Legion broadcast records for Gorek's lines.
INSERT INTO `creature_text` (`CreatureID`,`GroupID`,`ID`,`Text`,`Type`,`Language`,`Probability`,`BroadcastTextID`,`comment`)
VALUES
    (36662,0,0,'',12,0,100,32561,'Gorek - initial reaction to the detonator'),
    (36662,1,0,'',12,0,100,36612,'Gorek - tower burning')
ON DUPLICATE KEY UPDATE `BroadcastTextID`=`creature_text`.`BroadcastTextID`;

-- The 26972 objective catalog has no row for this event. Persist the actual
-- GO activation in an unused local objective row and leave existing POIs.
INSERT INTO `quest_objectives`
    (`ID`,`QuestID`,`Type`,`StorageIndex`,`ObjectID`,`Amount`,`Flags`,`Flags2`,`TaskStep`,`Description`,`Bugged`,`VerifiedBuild`)
SELECT 940006,14477,2,0,196870,1,0,0,0,'Detonate the Explosives',0,0
WHERE NOT EXISTS (SELECT 1 FROM `quest_objectives` WHERE `QuestID`=14477);

UPDATE `gameobject_template`
SET `ScriptName`='go_azshara_goblin_detonator'
WHERE `entry`=196870 AND `type`=1 AND `Data1`=43 AND `ScriptName` IN ('','go_azshara_goblin_detonator');
