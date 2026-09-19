-- The Deaths of Chromie (LFG 1534 / scenario 1351), map 1756.
-- The final 7.3.5 client contains all four scenario stages, eight timeline
-- attack criteria, five map floors, creature names/models and portal objects.
-- The released LegionCore world DB retained the WDB rows but omitted the
-- server-side creature templates and every map-1756 spawn.

DELETE FROM `instance_template` WHERE `map` = 1756;
INSERT INTO `instance_template` (`map`, `parent`, `script`, `allowMount`, `bonusChance`) VALUES
(1756, 0, 'instance_deaths_of_chromie', 1, 0);

DELETE FROM `scenario_data` WHERE `MapID` = 1756 OR `ScenarioID` = 1351;
INSERT INTO `scenario_data` (`ScenarioID`, `MapID`, `DifficultyID`, `Team`, `Class`, `LfgDungeonID`) VALUES
(1351, 1756, 12, 0, 0, 1534);

DELETE FROM `lfg_entrances` WHERE `dungeonId` = 1534;
INSERT INTO `lfg_entrances`
(`dungeonId`, `name`, `position_x`, `position_y`, `position_z`, `orientation`) VALUES
(1534, 'The Deaths of Chromie', 13670.22, 13062.12, 342.80, 0.91);

-- Rebuild the missing server-side rows from compatible templates. Their
-- authoritative names, types, display IDs, classification and health scaling
-- are already present in creature_template_wdb from build 26124.
DROP TEMPORARY TABLE IF EXISTS `_tmp_chromie_creature_template`;
CREATE TEMPORARY TABLE `_tmp_chromie_creature_template` LIKE `creature_template`;

INSERT INTO `_tmp_chromie_creature_template`
SELECT * FROM `creature_template` WHERE `entry` = 27856 LIMIT 1;
UPDATE `_tmp_chromie_creature_template` SET `entry` = 122510;
INSERT IGNORE INTO `creature_template` SELECT * FROM `_tmp_chromie_creature_template`;
UPDATE `_tmp_chromie_creature_template` SET `entry` = 123628;
INSERT IGNORE INTO `creature_template` SELECT * FROM `_tmp_chromie_creature_template`;
UPDATE `_tmp_chromie_creature_template` SET `entry` = 123722;
INSERT IGNORE INTO `creature_template` SELECT * FROM `_tmp_chromie_creature_template`;
UPDATE `_tmp_chromie_creature_template` SET `entry` = 123837;
INSERT IGNORE INTO `creature_template` SELECT * FROM `_tmp_chromie_creature_template`;
UPDATE `_tmp_chromie_creature_template` SET `entry` = 124154;
INSERT IGNORE INTO `creature_template` SELECT * FROM `_tmp_chromie_creature_template`;

DELETE FROM `_tmp_chromie_creature_template`;
INSERT INTO `_tmp_chromie_creature_template`
SELECT * FROM `creature_template` WHERE `entry` = 104496 LIMIT 1;
UPDATE `_tmp_chromie_creature_template` SET `entry` = 122553;
INSERT IGNORE INTO `creature_template` SELECT * FROM `_tmp_chromie_creature_template`;
UPDATE `_tmp_chromie_creature_template` SET `entry` = 122871;
INSERT IGNORE INTO `creature_template` SELECT * FROM `_tmp_chromie_creature_template`;
UPDATE `_tmp_chromie_creature_template` SET `entry` = 122874;
INSERT IGNORE INTO `creature_template` SELECT * FROM `_tmp_chromie_creature_template`;
UPDATE `_tmp_chromie_creature_template` SET `entry` = 122875;
INSERT IGNORE INTO `creature_template` SELECT * FROM `_tmp_chromie_creature_template`;
UPDATE `_tmp_chromie_creature_template` SET `entry` = 123021;
INSERT IGNORE INTO `creature_template` SELECT * FROM `_tmp_chromie_creature_template`;
UPDATE `_tmp_chromie_creature_template` SET `entry` = 123721;
INSERT IGNORE INTO `creature_template` SELECT * FROM `_tmp_chromie_creature_template`;
UPDATE `_tmp_chromie_creature_template` SET `entry` = 123836;
INSERT IGNORE INTO `creature_template` SELECT * FROM `_tmp_chromie_creature_template`;
UPDATE `_tmp_chromie_creature_template` SET `entry` = 124052;
INSERT IGNORE INTO `creature_template` SELECT * FROM `_tmp_chromie_creature_template`;
DROP TEMPORARY TABLE `_tmp_chromie_creature_template`;

UPDATE `creature_template`
SET `minlevel` = 112, `maxlevel` = 112, `HealthScalingExpansion` = 6,
    `faction` = 35, `npcflag` = 1, `unit_flags` = (`unit_flags` | 768),
    `ScriptName` = 'npc_deaths_of_chromie'
WHERE `entry` IN (122510, 123628, 123722, 123837, 124154);

UPDATE `creature_template`
SET `minlevel` = 112, `maxlevel` = 112, `HealthScalingExpansion` = 6,
    `faction` = 14, `npcflag` = 0, `unit_flags` = (`unit_flags` & ~33555202),
    `ScriptName` = 'npc_deaths_of_chromie_combat'
WHERE `entry` IN (122553, 122871, 122874, 122875, 123021, 123721, 123836, 124052);

UPDATE `gameobject_template`
SET `ScriptName` = 'go_deaths_of_chromie_portal'
WHERE `entry` IN (272379, 272381, 272382, 272383);

-- WorldMapArea 1177 and DungeonMap 1158-1162 provide the map/floor bounds.
-- The positions below are the archived Wowhead 7.3.5 map percentages converted
-- back to world coordinates and checked against the extracted map-1756 terrain.
DELETE FROM `creature` WHERE `map` = 1756;
INSERT INTO `creature`
(`id`, `map`, `zoneId`, `areaId`, `spawnMask`, `phaseMask`, `PhaseId`, `modelid`, `equipment_id`,
 `position_x`, `position_y`, `position_z`, `orientation`, `spawntimesecs`, `spawndist`, `currentwaypoint`,
 `curhealth`, `curmana`, `MovementType`, `npcflag`, `npcflag2`, `unit_flags`, `dynamicflags`, `AiID`,
 `MovementID`, `MeleeID`, `isActive`, `skipClone`, `personal_size`, `isTeemingSpawn`, `unit_flags3`) VALUES
(122510, 1756, 9051, 0, 0, 1, '', 0, 0, 13670.22, 13062.12,  342.80, 0.91, 7200, 0, 0, 1, 0, 0, 1, 0, 768, 0, 0, 0, 0, 1, 0, 0, 0, 0),

(122553, 1756, 9051, 0, 0, 1, '', 0, 0, 13240.16, 13258.41,   21.80, 0.00, 7200, 0, 0, 1, 0, 0, 0, 0,   0, 0, 0, 0, 0, 1, 0, 0, 0, 0),
(122871, 1756, 9051, 0, 0, 1, '', 0, 0, 14534.07, 14295.95,  126.50, 0.00, 7200, 0, 0, 1, 0, 0, 0, 0,   0, 0, 0, 0, 0, 1, 0, 0, 0, 0),
(122874, 1756, 9051, 0, 0, 1, '', 0, 0, 13864.68, 13751.94,   54.00, 0.00, 7200, 0, 0, 1, 0, 0, 0, 0,   0, 0, 0, 0, 0, 1, 0, 0, 0, 0),
(122875, 1756, 9051, 0, 0, 1, '', 0, 0, 12997.09, 12849.00, -155.80, 0.00, 7200, 0, 0, 1, 0, 0, 0, 0,   0, 0, 0, 0, 0, 1, 0, 0, 0, 0),

(124052, 1756, 9051, 0, 0, 1, '', 0, 0, 14231.68,  4804.27,   35.00, 0.00, 7200, 0, 0, 1, 0, 0, 0, 0,   0, 0, 0, 0, 0, 1, 0, 0, 0, 0),
(123021, 1756, 9051, 0, 0, 1, '', 0, 0,  8816.79,  4054.42,  968.00, 0.00, 7200, 0, 0, 1, 0, 0, 0, 0,   0, 0, 0, 0, 0, 1, 0, 0, 0, 0),
(123836, 1756, 9051, 0, 0, 1, '', 0, 0,  8702.87, 14812.73,  159.00, 0.00, 7200, 0, 0, 1, 0, 0, 0, 0,   0, 0, 0, 0, 0, 1, 0, 0, 0, 0),
(123721, 1756, 9051, 0, 0, 1, '', 0, 0,  2733.35, 10026.72,   34.00, 0.00, 7200, 0, 0, 1, 0, 0, 0, 0,   0, 0, 0, 0, 0, 1, 0, 0, 0, 0),

(124154, 1756, 9051, 0, 0, 1, '', 0, 0, 14182.94,  4924.67,   35.00, 0.00, 7200, 0, 0, 1, 0, 0, 1, 0, 768, 0, 0, 0, 0, 1, 0, 0, 0, 0),
(123628, 1756, 9051, 0, 0, 1, '', 0, 0,  8796.97,  4061.21,  968.00, 0.00, 7200, 0, 0, 1, 0, 0, 1, 0, 768, 0, 0, 0, 0, 1, 0, 0, 0, 0),
(123837, 1756, 9051, 0, 0, 1, '', 0, 0,  8696.87, 14834.10,  160.00, 0.00, 7200, 0, 0, 1, 0, 0, 1, 0, 768, 0, 0, 0, 0, 1, 0, 0, 0, 0),
(123722, 1756, 9051, 0, 0, 1, '', 0, 0,  2708.35,  9932.82,   34.00, 0.00, 7200, 0, 0, 1, 0, 0, 1, 0, 768, 0, 0, 0, 0, 1, 0, 0, 0, 0);

DELETE FROM `gameobject` WHERE `map` = 1756;
INSERT INTO `gameobject`
(`id`, `map`, `zoneId`, `areaId`, `spawnMask`, `phaseMask`, `PhaseId`, `position_x`, `position_y`,
 `position_z`, `orientation`, `rotation0`, `rotation1`, `rotation2`, `rotation3`, `spawntimesecs`,
 `animprogress`, `AiID`, `state`, `isActive`, `personal_size`) VALUES
(272379, 1756, 9051, 0, 0, 1, '', 13658.00, 13056.00, 342.80, 0.00000, 0, 0, 0.00000,  1.00000, 300, 0, 0, 1, 1, 0),
(272381, 1756, 9051, 0, 0, 1, '', 13662.00, 13052.00, 342.80, 1.57080, 0, 0, 0.70711,  0.70711, 300, 0, 0, 1, 1, 0),
(272382, 1756, 9051, 0, 0, 1, '', 13667.00, 13051.00, 342.80, 3.14159, 0, 0, 1.00000,  0.00000, 300, 0, 0, 1, 1, 0),
(272383, 1756, 9051, 0, 0, 1, '', 13672.00, 13053.00, 342.80, 4.71239, 0, 0, 0.70711, -0.70711, 300, 0, 0, 1, 1, 0);
