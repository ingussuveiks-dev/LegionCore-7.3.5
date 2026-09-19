-- The Coldridge Cataclysm (LFG 1481 / scenario 1292), map 1723.
-- The scenario was removed after the 7.2 PTR, but its 21 steps and criteria
-- remain in the 7.3.5 client. The missing Joren and final troll templates are
-- reconstructed from their archived creature IDs/models and existing source
-- creatures; placement follows the matching map-0 Coldridge Valley geometry.

DELETE FROM `instance_template` WHERE `map` = 1723;
INSERT INTO `instance_template` (`map`, `parent`, `script`, `allowMount`, `bonusChance`) VALUES
(1723, 0, 'instance_coldridge_cataclysm', 0, 0);

DELETE FROM `scenario_data` WHERE `MapID` = 1723 OR `ScenarioID` = 1292;
INSERT INTO `scenario_data` (`ScenarioID`, `MapID`, `DifficultyID`, `Team`, `Class`, `LfgDungeonID`) VALUES
(1292, 1723, 12, 469, 0, 1481);

DELETE FROM `lfg_entrances` WHERE `dungeonId` = 1481;
INSERT INTO `lfg_entrances`
(`dungeonId`, `name`, `position_x`, `position_y`, `position_z`, `orientation`) VALUES
(1481, 'The Coldridge Cataclysm', -6228.31, 331.55, 383.27, 2.60);

-- Joren 119296 and Monster of a Troll 119358 are still referenced by the
-- client criteria but their creature rows were removed from the final world DB.
-- Clone compatible originals before applying their archived names/models.
DROP TEMPORARY TABLE IF EXISTS `_tmp_coldridge_creature_template`;
CREATE TEMPORARY TABLE `_tmp_coldridge_creature_template` LIKE `creature_template`;
INSERT INTO `_tmp_coldridge_creature_template`
SELECT * FROM `creature_template` WHERE `entry` = 37081 LIMIT 1;
UPDATE `_tmp_coldridge_creature_template` SET `entry` = 119296;
INSERT IGNORE INTO `creature_template` SELECT * FROM `_tmp_coldridge_creature_template`;
DELETE FROM `_tmp_coldridge_creature_template`;
INSERT INTO `_tmp_coldridge_creature_template`
SELECT * FROM `creature_template` WHERE `entry` = 808 LIMIT 1;
UPDATE `_tmp_coldridge_creature_template` SET `entry` = 119358;
INSERT IGNORE INTO `creature_template` SELECT * FROM `_tmp_coldridge_creature_template`;
DROP TEMPORARY TABLE `_tmp_coldridge_creature_template`;

DROP TEMPORARY TABLE IF EXISTS `_tmp_coldridge_creature_wdb`;
CREATE TEMPORARY TABLE `_tmp_coldridge_creature_wdb` LIKE `creature_template_wdb`;
INSERT INTO `_tmp_coldridge_creature_wdb`
SELECT * FROM `creature_template_wdb` WHERE `Entry` = 37081 LIMIT 1;
UPDATE `_tmp_coldridge_creature_wdb`
SET `Entry` = 119296, `Name1` = 'Joren Ironstock', `Title` = '', `Displayid1` = 30505,
    `VerifiedBuild` = 23852;
INSERT IGNORE INTO `creature_template_wdb` SELECT * FROM `_tmp_coldridge_creature_wdb`;
DELETE FROM `_tmp_coldridge_creature_wdb`;
INSERT INTO `_tmp_coldridge_creature_wdb`
SELECT * FROM `creature_template_wdb` WHERE `Entry` = 808 LIMIT 1;
UPDATE `_tmp_coldridge_creature_wdb`
SET `Entry` = 119358, `Name1` = 'Monster of a Troll', `Title` = '<Almost Unbelievable>',
    `Displayid1` = 7803, `Displayid2` = 0, `Displayid3` = 0, `Displayid4` = 0,
    `Classification` = 1, `HpMulti` = 25, `VerifiedBuild` = 23852;
INSERT IGNORE INTO `creature_template_wdb` SELECT * FROM `_tmp_coldridge_creature_wdb`;
DROP TEMPORARY TABLE `_tmp_coldridge_creature_wdb`;

UPDATE `creature_template`
SET `minlevel` = 110, `maxlevel` = 110, `HealthScalingExpansion` = 6, `faction` = 35,
    `npcflag` = (`npcflag` | 1), `ScriptName` = 'npc_coldridge_story'
WHERE `entry` IN (119296, 37087);

UPDATE `creature_template`
SET `minlevel` = 110, `maxlevel` = 110, `HealthScalingExpansion` = 6, `faction` = 14,
    `unit_flags` = (`unit_flags` & ~33555202), `ScriptName` = 'npc_coldridge_story'
WHERE `entry` = 119358;

UPDATE `creature_template`
SET `ScriptName` = 'npc_coldridge_story'
WHERE `entry` = 1125;

UPDATE `gameobject_template`
SET `ScriptName` = 'go_coldridge_story_keg'
WHERE `entry` IN (201609, 201610, 201611);

DELETE FROM `creature` WHERE `map` = 1723;
INSERT INTO `creature`
(`id`, `map`, `zoneId`, `areaId`, `spawnMask`, `phaseMask`, `PhaseId`, `modelid`, `equipment_id`,
 `position_x`, `position_y`, `position_z`, `orientation`, `spawntimesecs`, `spawndist`, `currentwaypoint`,
 `curhealth`, `curmana`, `MovementType`, `npcflag`, `npcflag2`, `unit_flags`, `dynamicflags`, `AiID`,
 `MovementID`, `MeleeID`, `isActive`, `skipClone`, `personal_size`, `isTeemingSpawn`, `unit_flags3`) VALUES
(119296, 1723, 0, 0, 0, 1, '', 0, 0, -6228.31, 331.55, 383.27, 2.60, 7200, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0),
(786,    1723, 0, 0, 0, 1, '', 0, 0, -6219.40, 340.10, 383.30, 3.10, 7200, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0),
(37087,  1723, 0, 0, 0, 1, '', 0, 0, -6087.60, 386.03, 395.54, 1.81, 7200, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0),
(1354,   1723, 0, 0, 0, 1, '', 0, 0, -6216.20, 343.40, 383.20, 3.40, 7200, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0);

DELETE FROM `gameobject` WHERE `map` = 1723;
INSERT INTO `gameobject`
(`id`, `map`, `zoneId`, `areaId`, `spawnMask`, `phaseMask`, `PhaseId`, `position_x`, `position_y`,
 `position_z`, `orientation`, `rotation0`, `rotation1`, `rotation2`, `rotation3`, `spawntimesecs`,
 `animprogress`, `AiID`, `state`, `isActive`, `personal_size`) VALUES
(201611, 1723, 0, 0, 0, 1, '', -6334.00, 354.00, 379.00, 3.20, 0, 0, 0.99957, -0.02920, 300, 0, 0, 1, 1, 0),
(201609, 1723, 0, 0, 0, 1, '', -6360.00, 505.00, 386.00, 3.80, 0, 0, 0.94630, -0.32329, 300, 0, 0, 1, 1, 0),
(201610, 1723, 0, 0, 0, 1, '', -6490.00, 330.00, 369.00, 1.15, 0, 0, 0.54383, 0.83920, 300, 0, 0, 1, 1, 0);

DELETE FROM `creature_text` WHERE `CreatureID` = 786 AND `GroupID` IN (0, 1, 2, 3);
INSERT INTO `creature_text`
(`CreatureID`, `GroupID`, `ID`, `Text`, `Type`, `Language`, `Probability`, `Emote`, `Duration`,
 `Sound`, `BroadcastTextID`, `MinTimer`, `MaxTimer`, `SpellID`, `comment`) VALUES
(786, 0, 0, 'They took the kegs!', 14, 0, 100, 5, 0, 0, 127866, 0, 0, 0, 'Coldridge Cataclysm - Grelin panic'),
(786, 1, 0, 'Jona, Joren, go after the Gnomenbrau! A group of troggs carried it off!', 12, 0, 100, 25, 0, 0, 127867, 0, 0, 0, 'Coldridge Cataclysm - Gnomenbrau'),
(786, 2, 0, 'Soren lad, find the boar that stole our Theramore Pale!', 12, 0, 100, 25, 0, 0, 127868, 0, 0, 0, 'Coldridge Cataclysm - Theramore Pale'),
(786, 3, 0, 'I''ll chase the trolls to their cave and find the Stormhammer Stout!', 12, 0, 100, 25, 0, 0, 127869, 0, 0, 0, 'Coldridge Cataclysm - Stormhammer Stout');
