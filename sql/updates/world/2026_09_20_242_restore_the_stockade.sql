-- Restore The Stockade (map 34) for Legion 7.3.5.
-- Boss scripts and dialogue are based on the Trinity/Ashamane implementation.
-- Trash abilities come from the Cataclysm/Mists NPC ability records retained for these entries.

UPDATE `instance_template`
SET `script` = 'instance_the_stockade', `allowMount` = 0
WHERE `map` = 34;

-- Correct the encounter credit entry so killing Hogger completes the encounter.
UPDATE `instance_encounters`
SET `creditEntry` = 46254, `comment` = 'Hogger'
WHERE `entry` = 1144 AND `difficulty` = 1;

UPDATE `creature_template` SET `AIName` = '', `ScriptName` = 'boss_randolph_moloch' WHERE `entry` = 46383;
UPDATE `creature_template` SET `AIName` = '', `ScriptName` = 'boss_lord_overheat' WHERE `entry` = 46264;
UPDATE `creature_template` SET `AIName` = '', `ScriptName` = 'boss_hogger' WHERE `entry` = 46254;
UPDATE `creature_template` SET `AIName` = '', `ScriptName` = 'npc_warden_thelwater' WHERE `entry` = 46409;
UPDATE `creature_template` SET `AIName` = '', `ScriptName` = 'npc_mortimer_moloch' WHERE `entry` = 46482;

UPDATE `creature_template`
SET `AIName` = '', `ScriptName` = 'npc_the_stockade_trash'
WHERE `entry` IN (46248, 46249, 46250, 46251, 46252, 46260, 46261, 46262, 46263, 46375, 46379, 46381);

-- Bruegal Ironknuckle is the one Stockade creature handled by the official 7.3.5 SmartAI data.
UPDATE `creature_template` SET `AIName` = 'SmartAI', `ScriptName` = '' WHERE `entry` = 1720;
DELETE FROM `smart_scripts` WHERE `entryorguid` = 1720 AND `source_type` = 0;
INSERT INTO `smart_scripts`
(`entryorguid`,`source_type`,`id`,`link`,`Difficulties`,`event_type`,`event_phase_mask`,`event_chance`,`event_flags`,`event_param1`,`event_param2`,`event_param3`,`event_param4`,`event_param5`,`action_type`,`action_param1`,`action_param2`,`action_param3`,`action_param4`,`action_param5`,`action_param6`,`target_type`,`target_param1`,`target_param2`,`target_param3`,`target_param4`,`target_x`,`target_y`,`target_z`,`target_o`,`comment`) VALUES
(1720,0,0,0,'',2,0,100,1,0,15,0,0,0,25,1,0,0,0,0,0,7,0,0,0,0,0,0,0,0,'Bruegal Ironknuckle - Between 0-15% Health - Flee For Assist (No Repeat)');

-- Replace the incompatible localized group numbering with the original broadcast-text groups used by the scripts.
DELETE FROM `creature_text` WHERE `CreatureID` IN (46254, 46264, 46383, 46409, 46482);
INSERT INTO `creature_text`
(`CreatureID`,`GroupID`,`ID`,`Text`,`Type`,`Language`,`Probability`,`Emote`,`Duration`,`Sound`,`BroadcastTextID`,`MinTimer`,`MaxTimer`,`SpellID`,`comment`) VALUES
(46254,0,0,'Forest just setback!',14,0,100,0,0,0,46474,0,0,0,'Hogger - SAY_PULL'),
(46254,1,0,'%s Enrages!',41,0,100,0,0,0,46631,0,0,0,'Hogger - SAY_ENRAGE'),
(46254,2,0,'Yiiipe!',14,0,100,0,0,0,46476,0,0,0,'Hogger - SAY_DEATH'),
(46264,0,0,'ALL MUST BURN!',14,0,100,0,0,0,46487,0,0,0,'Lord Overheat - SAY_PULL'),
(46264,1,0,'FIRE... EXTINGUISHED!',14,0,100,0,0,0,46489,0,0,0,'Lord Overheat - SAY_DEATH'),
(46383,0,0,'Allow me to introduce myself. I am Randolph Moloch, and I will be killing you all today.',14,0,100,0,0,0,46495,0,0,0,'Randolph Moloch - SAY_PULL'),
(46383,1,0,'%s vanishes!',41,0,100,0,0,0,46630,0,0,0,'Randolph Moloch - SAY_VANISH'),
(46383,2,0,'My epic schemes, my great plans! Gone!',14,0,100,0,0,0,46497,0,0,0,'Randolph Moloch - SAY_DEATH'),
(46409,0,0,'This ends here, Hogger!',14,0,100,0,0,0,46571,0,0,0,'Warden Thelwater - SAY_WARDEN_1'),
(46409,1,0,'He''s...he''s dead?',12,0,100,0,0,0,46572,0,0,0,'Warden Thelwater - SAY_WARDEN_2'),
(46409,2,0,'It''s simply too good to be true. You couldn''t have killed him so easily!',12,0,100,0,0,0,46573,0,0,0,'Warden Thelwater - SAY_WARDEN_3'),
(46482,0,0,'%s collapses from a heart attack!',16,0,100,0,0,0,46561,0,0,0,'Mortimer Moloch - SAY_DEATH'),
(46482,1,0,'Egad! My sophisticated heart!',14,0,100,0,0,0,46559,0,0,0,'Mortimer Moloch - SAY_EMOTE');

-- Entry 1708 belongs to the pre-Cataclysm dungeon population and is absent in TDB735.
DELETE `ca` FROM `creature_addon` `ca`
INNER JOIN `creature` `c` ON `c`.`guid` = `ca`.`guid`
WHERE `c`.`map` = 34 AND `c`.`id` = 1708;
DELETE `ss` FROM `smart_scripts` `ss`
INNER JOIN `creature` `c` ON `ss`.`entryorguid` = -CAST(`c`.`guid` AS SIGNED) AND `ss`.`source_type` = 0
WHERE `c`.`map` = 34 AND `c`.`id` = 1708;
DELETE FROM `creature` WHERE `map` = 34 AND `id` = 1708;

-- Restore both invisible summon-enabler stalkers present in the official Legion world data.
INSERT INTO `creature`
(`id`,`map`,`spawnMask`,`phaseMask`,`PhaseId`,`position_x`,`position_y`,`position_z`,`orientation`,`spawntimesecs`,`spawndist`,`MovementType`)
SELECT 53488,34,2,1,'',66.8395,0.820416,-22.9799,0,7200,0,0
WHERE NOT EXISTS (SELECT 1 FROM `creature` WHERE `map` = 34 AND `id` = 53488 AND ABS(`position_x` - 66.8395) < 0.01 AND ABS(`position_y` - 0.820416) < 0.01);
INSERT INTO `creature`
(`id`,`map`,`spawnMask`,`phaseMask`,`PhaseId`,`position_x`,`position_y`,`position_z`,`orientation`,`spawntimesecs`,`spawndist`,`MovementType`)
SELECT 53488,34,2,1,'',76.7849,0.891,-25.5229,0,7200,0,0
WHERE NOT EXISTS (SELECT 1 FROM `creature` WHERE `map` = 34 AND `id` = 53488 AND ABS(`position_x` - 76.7849) < 0.01 AND ABS(`position_y` - 0.891) < 0.01);
UPDATE `creature` SET `spawnMask` = 2 WHERE `map` = 34 AND `id` = 53488;

-- Reinstate four random patrols that had been converted to stationary spawns.
UPDATE `creature` SET `spawndist` = 5, `MovementType` = 1 WHERE `guid` IN (167220,167236,167246,167287) AND `map` = 34;

-- Reinstate twelve geometry-matched waypoint patrols. The source dump's addon GUID links were stale,
-- so these paths are deliberately matched to the actual 7.3.5 spawn coordinates.
UPDATE `creature` SET `spawndist` = 0, `MovementType` = 2, `currentwaypoint` = 0
WHERE `guid` IN (167214,167216,167219,167273,167275,167279,167283,167285,167289,167306,167307,167342) AND `map` = 34;

DELETE FROM `creature_addon` WHERE `guid` IN (167214,167216,167219,167273,167275,167279,167283,167285,167289,167306,167307,167342);
INSERT INTO `creature_addon` (`guid`,`path_id`) VALUES
(167214,1672140),(167216,1672160),(167219,1672190),(167273,1672730),(167275,1672750),(167279,1672790),
(167283,1672830),(167285,1672850),(167289,1672890),(167306,1673060),(167307,1673070),(167342,1673420);

DELETE FROM `waypoint_data` WHERE `id` IN (1672140,1672160,1672190,1672730,1672750,1672790,1672830,1672850,1672890,1673060,1673070,1673420);
INSERT INTO `waypoint_data`
(`id`,`point`,`position_x`,`position_y`,`position_z`,`orientation`,`delay`,`delay_chance`,`move_type`,`speed`,`action`,`action_chance`,`entry`,`wpguid`) VALUES
(1673070,1,110.294,-18.2778,-26.5229,0,0,0,0,0,0,100,0,0),
(1673070,2,110.703,-19.1904,-26.5229,0,0,0,0,0,0,100,0,0),
(1673070,3,108.707,-14.7353,-26.5229,0,0,0,0,0,0,100,0,0),
(1673070,4,108.707,-14.7353,-26.5229,0,0,0,0,0,0,100,0,0),
(1673060,1,132.866,1.68494,-25.6062,0,0,0,0,0,0,100,0,0),
(1673060,2,132.009,1.1699,-25.6062,0,0,0,0,0,0,100,0,0),
(1673060,3,131.334,1.63358,-25.6062,0,0,0,0,0,0,100,0,0),
(1673060,4,131.334,1.63358,-25.6062,0,0,0,0,0,0,100,0,0),
(1672890,1,105.652,-28.5862,-26.5229,0,0,0,0,0,0,100,0,0),
(1672890,2,104.795,-29.1012,-26.5229,0,0,0,0,0,0,100,0,0),
(1672890,3,105.953,-27.7429,-26.5229,0,0,0,0,0,0,100,0,0),
(1672890,4,105.953,-27.7429,-26.5229,0,0,0,0,0,0,100,0,0),
(1672850,1,147.865,-50.7881,-34.8562,0,0,0,0,0,0,100,0,0),
(1672850,2,146.868,-50.8707,-34.8562,0,0,0,0,0,0,100,0,0),
(1672850,3,146.571,-49.5761,-34.8562,0,0,0,0,0,0,100,0,0),
(1672850,4,146.571,-49.5761,-34.8562,0,0,0,0,0,0,100,0,0),
(1672830,1,153.951,-47.9694,-34.8562,0,0,0,0,0,0,100,0,0),
(1672830,2,154.64,-48.6941,-34.8562,0,0,0,0,0,0,100,0,0),
(1672830,3,153.149,-48.3541,-34.8562,0,0,0,0,0,0,100,0,0),
(1672830,4,151.191,-47.9077,-34.8562,0,0,0,0,0,0,100,0,0),
(1672830,5,151.191,-47.9077,-34.8562,0,0,0,0,0,0,100,0,0),
(1672790,1,125.32,-62.541,-33.9396,0,0,0,0,0,0,100,0,0),
(1672790,2,125.86,-61.6993,-33.9396,0,0,0,0,0,0,100,0,0),
(1672790,3,126.255,-61.0834,-33.9396,0,0,0,0,0,0,100,0,0),
(1672790,4,128.001,-58.3609,-33.9396,0,0,0,0,0,0,100,0,0),
(1672790,5,130.018,-51.5804,-33.9396,0,0,0,0,0,0,100,0,0),
(1672790,6,130.34,-50.4989,-33.9396,0,0,0,0,0,0,100,0,0),
(1672790,7,129.576,-45.469,-33.9396,0,0,0,0,0,0,100,0,0),
(1672790,8,129.426,-44.4811,-33.9396,0,0,0,0,0,0,100,0,0),
(1672790,9,129.426,-44.4811,-33.9396,0,0,0,0,0,0,100,0,0),
(1672750,1,162.874,101.41,-34.9649,0,0,0,0,0,0,100,0,0),
(1672750,2,161.891,101.591,-34.9649,0,0,0,0,0,0,100,0,0),
(1672750,3,158.366,102.24,-35.1896,0,0,0,0,0,0,100,0,0),
(1672750,4,152.103,100.327,-35.1896,0,0,0,0,0,0,100,0,0),
(1672750,5,150.572,99.8595,-35.1896,0,0,0,0,0,0,100,0,0),
(1672750,6,150.572,99.8595,-35.1896,0,0,0,0,0,0,100,0,0),
(1672730,1,181.549,91.3863,-33.9396,0,0,0,0,0,0,100,0,0),
(1672730,2,181.055,90.5167,-33.9396,0,0,0,0,0,0,100,0,0),
(1672730,3,176.763,91.3261,-33.9396,0,0,0,0,0,0,100,0,0),
(1672730,4,176.763,91.3261,-33.9396,0,0,0,0,0,0,100,0,0),
(1672190,1,122.621,-65.386,-33.9378,0,0,0,0,0,0,100,0,0),
(1672190,2,121.682,-65.0414,-33.9378,0,0,0,0,0,0,100,0,0),
(1672190,3,121.157,-64.8486,-33.9374,0,0,0,0,0,0,100,0,0),
(1672190,4,119.006,-64.059,-34.8494,0,0,0,0,0,0,100,0,0),
(1672190,5,117.944,-63.6694,-34.856,0,0,0,0,0,0,100,0,0),
(1672190,6,114.462,-61.8837,-34.8562,0,0,0,0,0,0,100,0,0),
(1672190,7,111.293,-60.2584,-34.8562,0,0,0,0,0,0,100,0,0),
(1672190,8,110.055,-60.4123,-34.8562,0,0,0,0,0,0,100,0,0),
(1672190,9,104.609,-61.0894,-34.8562,0,0,0,0,0,0,100,0,0),
(1672190,10,104.609,-61.0894,-34.8562,0,0,0,0,0,0,100,0,0),
(1672160,1,100.193,-38.7373,-34.8562,0,0,0,0,0,0,100,0,0),
(1672160,2,100.307,-39.7309,-34.8562,0,0,0,0,0,0,100,0,0),
(1672160,3,99.3503,-38.8146,-34.8562,0,0,0,0,0,0,100,0,0),
(1672160,4,99.3503,-38.8146,-34.8562,0,0,0,0,0,0,100,0,0),
(1672140,1,147.326,103.736,-35.1896,0,0,0,0,0,0,100,0,0),
(1672140,2,147.363,102.737,-35.1896,0,0,0,0,0,0,100,0,0),
(1672140,3,147.419,101.211,-35.1896,0,0,0,0,0,0,100,0,0),
(1672140,4,146.919,99.3285,-35.1896,0,0,0,0,0,0,100,0,0),
(1672140,5,146.392,97.3472,-35.1896,0,0,0,0,0,0,100,0,0),
(1672140,6,146.056,96.0809,-34.7753,0,0,0,0,0,0,100,0,0),
(1672140,7,145.617,94.4278,-33.9411,0,0,0,0,0,0,100,0,0),
(1672140,8,145.173,92.758,-33.9396,0,0,0,0,0,0,100,0,0),
(1672140,9,145.173,92.758,-33.9396,0,0,0,0,0,0,100,0,0),
(1673420,1,84.4642,33.8602,-26.442,0,0,0,0,0,0,100,0,0),
(1673420,2,84.4642,32.8602,-26.442,0,0,0,0,0,0,100,0,0),
(1673420,3,82.5378,32.5624,-26.5259,0,0,0,0,0,0,100,0,0),
(1673420,4,80.4382,32.1262,-26.5265,0,0,0,0,0,0,100,0,0),
(1673420,5,79.0513,30.7565,-26.5262,0,0,0,0,0,0,100,0,0),
(1673420,6,78.5776,28.8914,-26.5229,0,0,0,0,0,0,100,0,0),
(1673420,7,78.9996,27.247,-26.5229,0,0,0,0,0,0,100,0,0),
(1673420,8,81.4716,26.7795,-26.5298,0,0,0,0,0,0,100,0,0),
(1673420,9,83.6806,26.924,-26.5335,0,0,0,0,0,0,100,0,0),
(1673420,10,86.3659,26.8395,-26.5267,0,0,0,0,0,0,100,0,0),
(1673420,11,88.4547,26.7095,-26.5229,0,0,0,0,0,0,100,0,0),
(1673420,12,90.2688,26.9445,-26.5229,0,0,0,0,0,0,100,0,0),
(1673420,13,91.2195,28.3116,-26.5229,0,0,0,0,0,0,100,0,0),
(1673420,14,91.276,29.5049,-26.5229,0,0,0,0,0,0,100,0,0),
(1673420,15,90.8135,30.8675,-26.5229,0,0,0,0,0,0,100,0,0),
(1673420,16,89.6863,31.783,-26.5229,0,0,0,0,0,0,100,0,0),
(1673420,17,88.7407,32.3032,-26.5229,0,0,0,0,0,0,100,0,0),
(1673420,18,87.2978,32.709,-26.5229,0,0,0,0,0,0,100,0,0),
(1673420,19,85.5401,32.9464,-26.5229,0,0,0,0,0,0,100,0,0),
(1673420,20,84.5596,32.7557,-26.5251,0,0,0,0,0,0,100,0,0),
(1673420,21,84.4853,31.0094,-26.5272,0,0,0,0,0,0,100,0,0),
(1673420,22,84.4622,29.6961,-26.5286,0,0,0,0,0,0,100,0,0),
(1673420,23,84.4622,29.6961,-26.5286,0,0,0,0,0,0,100,0,0);
