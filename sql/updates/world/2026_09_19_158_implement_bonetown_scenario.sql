-- Bonetown (LFG 770 / scenario 323), map 1200.
-- Scenario objectives, actors and dialogue are preserved in the 7.3.5 client
-- DB2/hotfix data. Spawn placement follows the matching Grom'gar geometry on
-- map 1116 and the public in-game Grom'gar coordinates.

DELETE FROM `instance_template` WHERE `map` = 1200;
INSERT INTO `instance_template` (`map`, `parent`, `script`, `allowMount`, `bonusChance`) VALUES
(1200, 0, 'instance_bonetown', 0, 0);

DELETE FROM `scenario_data` WHERE `MapID` = 1200 OR `ScenarioID` = 323;
INSERT INTO `scenario_data` (`ScenarioID`, `MapID`, `DifficultyID`, `Team`, `Class`, `LfgDungeonID`) VALUES
(323, 1200, 12, 67, 0, 770);

DELETE FROM `lfg_entrances` WHERE `dungeonId` = 770;
INSERT INTO `lfg_entrances`
(`dungeonId`, `name`, `position_x`, `position_y`, `position_z`, `orientation`) VALUES
(770, 'Bonetown Scenario', 5862.00, 3664.00, 121.00, 5.48);

DELETE FROM `creature` WHERE `map` = 1200;
INSERT INTO `creature`
(`id`, `map`, `zoneId`, `areaId`, `spawnMask`, `phaseMask`, `PhaseId`, `modelid`, `equipment_id`,
 `position_x`, `position_y`, `position_z`, `orientation`, `spawntimesecs`, `spawndist`, `currentwaypoint`,
 `curhealth`, `curmana`, `MovementType`, `npcflag`, `npcflag2`, `unit_flags`, `dynamicflags`, `AiID`,
 `MovementID`, `MeleeID`, `isActive`, `skipClone`, `personal_size`, `isTeemingSpawn`, `unit_flags3`) VALUES
(76353, 1200, 0, 0, 0, 1, '', 0, 0, 5857.80, 3658.70, 121.00, 5.48, 7200, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0),
(76330, 1200, 0, 0, 0, 1, '', 0, 0, 5613.94, 3552.10, 112.58, 4.74, 7200, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0),
(76312, 1200, 0, 0, 0, 1, '', 0, 0, 5818.40, 3641.80, 120.90, 0.10, 7200, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0),
(76313, 1200, 0, 0, 0, 1, '', 0, 0, 5834.20, 3635.10, 120.90, 2.80, 7200, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0),
(76316, 1200, 0, 0, 0, 1, '', 0, 0, 5827.80, 3651.20, 120.90, 4.70, 7200, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0),
(76317, 1200, 0, 0, 0, 1, '', 0, 0, 5597.40, 3465.61, 116.98, 1.63, 7200, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0);

-- The scenario reuses the same Grom'gar map geometry. These objects are the
-- matching outer ritual fires and inner-chamber braziers/bonfire from map 1116.
DELETE FROM `gameobject` WHERE `map` = 1200;
INSERT INTO `gameobject`
(`id`, `map`, `zoneId`, `areaId`, `spawnMask`, `phaseMask`, `PhaseId`, `position_x`, `position_y`,
 `position_z`, `orientation`, `rotation0`, `rotation1`, `rotation2`, `rotation3`, `spawntimesecs`,
 `animprogress`, `AiID`, `state`, `isActive`, `personal_size`) VALUES
(236481, 1200, 0, 0, 0, 1, '', 5846.96, 3612.47, 120.901, 3.38594, 0, 0, 0.99254, -0.12191, 300, 0, 0, 1, 0, 0),
(236482, 1200, 0, 0, 0, 1, '', 5827.88, 3589.79, 120.896, 1.60570, 0, 0, 0.71932, 0.69468, 300, 0, 0, 1, 0, 0),
(239302, 1200, 0, 0, 0, 1, '', 5598.61, 3518.46, 110.784, 0.00000, 0, 0, 0.00000, 1.00000, 300, 0, 0, 1, 0, 0),
(239303, 1200, 0, 0, 0, 1, '', 5616.17, 3486.54, 113.515, 0.00000, 0, 0, 0.00000, 1.00000, 300, 0, 0, 1, 0, 0),
(239304, 1200, 0, 0, 0, 1, '', 5579.33, 3486.97, 113.475, 5.59897, 0, 0, 0.33544, -0.94206, 300, 0, 0, 1, 0, 0),
(237593, 1200, 0, 0, 0, 1, '', 5597.58, 3430.04, 131.646, 1.57080, 0, 0, 0.70711, 0.70711, 300, 0, 0, 1, 0, 0);

UPDATE `creature_template`
SET `minlevel` = 100, `maxlevel` = 100, `HealthScalingExpansion` = 5,
    `faction` = 14, `unit_flags` = (`unit_flags` & ~33555202),
    `ScriptName` = 'npc_bonetown_combat'
WHERE `entry` IN (76312, 76313, 76316, 76317, 76318, 76322, 76323, 76324, 76325, 76327);

UPDATE `creature_template`
SET `minlevel` = 100, `maxlevel` = 100, `HealthScalingExpansion` = 5,
    `faction` = 35, `unit_flags` = (`unit_flags` & ~33555202), `ScriptName` = ''
WHERE `entry` IN (76330, 76353);

DELETE FROM `creature_text` WHERE `CreatureID` IN (76317, 76330, 76353);
INSERT INTO `creature_text`
(`CreatureID`, `GroupID`, `ID`, `Text`, `Type`, `Language`, `Probability`, `Emote`, `Duration`,
 `Sound`, `BroadcastTextID`, `MinTimer`, `MaxTimer`, `SpellID`, `comment`) VALUES
(76353, 0, 0, 'Many Frostwolf have fallen here.', 12, 0, 100, 1, 0, 0, 79279, 0, 0, 0, 'Bonetown - Durotan intro'),
(76353, 1, 0, 'They''re using the souls of my people to raise the Colossal as a weapon.', 12, 0, 100, 1, 0, 0, 79280, 0, 0, 0, 'Bonetown - Durotan ritual'),
(76353, 2, 0, '$n, destroy these Soul Shaman and stop the ritual.', 12, 0, 100, 25, 0, 0, 79281, 0, 0, 0, 'Bonetown - Durotan objective'),
(76353, 3, 0, 'My warriors and I will rescue our brethren that still live.', 12, 0, 100, 1, 0, 0, 79282, 0, 0, 0, 'Bonetown - Durotan rescue'),
(76353, 4, 0, 'We must breach the inner chamber. Kilrogg cannot get away with this!', 12, 0, 100, 1, 0, 0, 79390, 0, 0, 0, 'Bonetown - Durotan inner chamber'),
(76353, 5, 0, 'Kilrogg! I should have known you were behind this.', 12, 0, 100, 5, 0, 0, 79385, 0, 0, 0, 'Bonetown - Durotan confronts Kilrogg'),
(76330, 0, 0, 'We have driven the Iron Horde back, $p, but the inner chamber remains sealed.', 12, 0, 100, 1, 0, 0, 79378, 0, 0, 0, 'Bonetown - Thrall at barrier'),
(76330, 1, 0, 'Stand back!', 14, 0, 100, 5, 0, 46359, 79377, 0, 0, 0, 'Bonetown - Thrall begins breach'),
(76330, 2, 0, '$p, defend me while I destroy the barrier!', 14, 0, 100, 25, 0, 0, 79391, 0, 0, 0, 'Bonetown - defend Thrall'),
(76330, 3, 0, 'I can bring it down but I need time to harness the elements.', 12, 0, 100, 1, 0, 0, 79383, 0, 0, 0, 'Bonetown - Thrall channels'),
(76330, 4, 0, 'The might of my ancestors flows through me!', 14, 0, 100, 5, 0, 0, 79384, 0, 0, 0, 'Bonetown - Thrall completes breach'),
(76330, 5, 0, 'It seems the elements of Draenor do not respond to me as they do on Azeroth...', 12, 0, 100, 1, 0, 47062, 79379, 0, 0, 0, 'Bonetown - Thrall exhausted'),
(76330, 6, 0, 'Go on ahead. Durotan awaits. I must catch my breath.', 12, 0, 100, 1, 0, 46360, 79380, 0, 0, 0, 'Bonetown - Thrall sends player ahead'),
(76317, 0, 0, 'You were fools to come here. I will rip the souls from your bodies and raise the Colossal to crush your feeble clan!', 14, 0, 100, 5, 0, 0, 79387, 0, 0, 0, 'Bonetown - Kilrogg challenge');
