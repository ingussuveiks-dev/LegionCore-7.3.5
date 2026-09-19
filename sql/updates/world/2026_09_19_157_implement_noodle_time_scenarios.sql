-- Finding the Secret Ingredient (LFG 745 / scenario 269) and
-- Noodle Time (LFG 749 / scenario 278), map 1157.
-- Static map data is restored from the public SkyFire/Legends 5.4.8 world
-- database. Scenario mechanics and IDs are taken from the 7.3.5 client DB2.

UPDATE `instance_template`
SET `script` = 'instance_noodle_time', `allowMount` = 0
WHERE `map` = 1157;

DELETE FROM `scenario_data` WHERE `MapID` = 1157 AND `ScenarioID` IN (269, 278);
INSERT INTO `scenario_data` (`ScenarioID`, `MapID`, `DifficultyID`, `Team`, `Class`, `LfgDungeonID`) VALUES
(269, 1157, 12, 0, 0, 745),
(278, 1157, 11, 0, 0, 749);

DELETE FROM `lfg_entrances` WHERE `dungeonId` IN (745, 749);
INSERT INTO `lfg_entrances` (`dungeonId`, `name`, `position_x`, `position_y`, `position_z`, `orientation`) VALUES
(745, 'Finding the Secret Ingredient', -268.498, 599.595, 167.548, 6.20845),
(749, 'Noodle Time', -268.498, 599.595, 167.548, 6.20845);

DELETE FROM `creature` WHERE `map` = 1157;
INSERT INTO `creature`
(`id`, `map`, `zoneId`, `areaId`, `spawnMask`, `phaseMask`, `PhaseId`, `modelid`, `equipment_id`,
 `position_x`, `position_y`, `position_z`, `orientation`, `spawntimesecs`, `spawndist`, `currentwaypoint`,
 `curhealth`, `curmana`, `MovementType`, `npcflag`, `npcflag2`, `unit_flags`, `dynamicflags`, `AiID`,
 `MovementID`, `MeleeID`, `isActive`, `skipClone`, `personal_size`, `isTeemingSpawn`, `unit_flags3`) VALUES
(73316, 1157, 0, 0, 0, 1, '', 0, 0, -268.498, 599.595, 167.548, 6.20845,
 7200, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0);

DELETE FROM `gameobject` WHERE `map` = 1157;
INSERT INTO `gameobject`
(`id`, `map`, `zoneId`, `areaId`, `spawnMask`, `phaseMask`, `PhaseId`, `position_x`, `position_y`,
 `position_z`, `orientation`, `rotation0`, `rotation1`, `rotation2`, `rotation3`, `spawntimesecs`,
 `animprogress`, `AiID`, `state`, `isActive`, `personal_size`) VALUES
(221726, 1157, 0, 0, 0, 1, '', -258.413, 597.519, 167.548, 1.33318, 0, 0, 0.618308, 0.785936, 300, 0, 0, 1, 0, 0),
(221726, 1157, 0, 0, 0, 1, '', -263.689, 601.899, 167.548, 1.33318, 0, 0, 0.618308, 0.785936, 300, 0, 0, 1, 0, 0),
(221726, 1157, 0, 0, 0, 1, '', -252.592, 598.724, 167.548, 1.33318, 0, 0, 0.618308, 0.785936, 300, 0, 0, 1, 0, 0),
(221726, 1157, 0, 0, 0, 1, '', -257.276, 602.899, 167.548, 1.33318, 0, 0, 0.618308, 0.785936, 300, 0, 0, 1, 0, 0),
(221724, 1157, 0, 0, 0, 1, '', -264.075, 600.361, 167.548, 1.33318, 0, 0, 0.618308, 0.785936, 300, 0, 0, 1, 0, 0),
(221724, 1157, 0, 0, 0, 1, '', -252.904, 597.307, 167.548, 1.33318, 0, 0, 0.618308, 0.785936, 300, 0, 0, 1, 0, 0),
(223742, 1157, 0, 0, 0, 1, '', -265.203, 611.988, 167.548, 2.05162, 0, 0, 0.855134, 0.518407, 300, 0, 0, 1, 0, 0),
(216310, 1157, 0, 0, 0, 1, '', -222.747, 526.270, 190.891, 1.03847, 0, 0, 0.496216, 0.868199, 300, 0, 0, 1, 0, 0),
(214163, 1157, 0, 0, 0, 1, '', 474.229, -272.371, 210.875, 5.11083, 0, 0, -0.553181, 0.833061, 300, 0, 0, 1, 0, 0),
(221738, 1157, 0, 0, 0, 1, '', -229.939, 506.901, 190.912, 5.77648, 0, 0, -0.250650, 0.968078, 300, 0, 0, 1, 0, 0),
(211976, 1157, 0, 0, 0, 1, '', -738.260, 1288.930, 116.461, 1.84132, 0, 0, 0.796001, 0.605295, 300, 0, 0, 1, 0, 0),
(211977, 1157, 0, 0, 0, 1, '', -781.651, 1378.070, 126.857, 1.84132, 0, 0, 0.796001, 0.605295, 300, 0, 0, 1, 0, 0),
(211981, 1157, 0, 0, 0, 1, '', -756.273, 1427.160, 139.657, 0.270525, 0, 0, 0.134851, 0.990866, 300, 0, 0, 1, 0, 0),
(211982, 1157, 0, 0, 0, 1, '', -794.358, 1416.600, 139.657, 0.270525, 0, 0, 0.134851, 0.990866, 300, 0, 0, 1, 0, 0);

UPDATE `creature_template`
SET `npcflag` = (`npcflag` | 16777216), `ScriptName` = 'npc_noodle_time_interaction'
WHERE `entry` IN
(72913, 72914, 72915, 72916, 72917, 72918, 72919, 72920, 72921, 72922, 72923, 72924,
 72925, 72926, 72928, 72948, 72949, 72950, 72951, 72961, 72962, 72963);

UPDATE `creature_template`
SET `npcflag` = (`npcflag` | 1), `ScriptName` = 'npc_noodle_time_interaction'
WHERE `entry` = 73316;

UPDATE `creature_template`
SET `spell1` = 146421, `spell2` = 146422, `spell3` = 146423,
    `spell4` = 146519, `spell5` = 146521
WHERE `entry` = 72952;

DELETE FROM `spell_script_names`
WHERE `spell_id` IN (146421, 146422, 146423, 146519, 146521)
  AND `ScriptName` IN ('spell_noodle_time_cook', 'spell_noodle_time_clear');
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(146421, 'spell_noodle_time_cook'),
(146422, 'spell_noodle_time_cook'),
(146423, 'spell_noodle_time_cook'),
(146519, 'spell_noodle_time_clear'),
(146521, 'spell_noodle_time_clear');

DELETE FROM `creature_text` WHERE `CreatureID` = 73316 AND `GroupID` IN (0, 1, 2);
INSERT INTO `creature_text`
(`CreatureID`, `GroupID`, `ID`, `Text`, `Type`, `Language`, `Probability`, `Emote`, `Duration`,
 `Sound`, `BroadcastTextID`, `MinTimer`, `MaxTimer`, `SpellID`, `comment`) VALUES
(73316, 0, 0, 'Welcome, $n. Customers will line up and wait for you to tell them to sit down. Match each patron to the soup they prefer!', 12, 0, 100, 1, 0, 0, 0, 0, 0, 0, 'Noodle Time - round begins'),
(73316, 1, 0, 'Well done!', 12, 0, 100, 4, 0, 0, 0, 0, 0, 0, 'Noodle Time - success'),
(73316, 2, 0, 'You have let too many customers get away. Let us set the stand up and try again.', 12, 0, 100, 18, 0, 0, 0, 0, 0, 0, 'Noodle Time - retry');
