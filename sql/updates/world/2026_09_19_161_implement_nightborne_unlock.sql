-- The Nightborne (LFG 1634 / scenario 1423), map 1812.
-- Final 7.3.5 DB2 data defines one step: CriteriaTree 60877/60878,
-- "Speak with Arluin", using SCRIPT_EVENT_2 asset 60314. Map 1812 is the
-- SuramarNightborneUnlock child of Suramar (1220), so it shares Suramar's
-- coordinate space. The released world DB omitted all map-1812 static data.

DELETE FROM `instance_template` WHERE `map` = 1812;
INSERT INTO `instance_template` (`map`, `parent`, `script`, `allowMount`, `bonusChance`) VALUES
(1812, 0, 'instance_nightborne_unlock', 0, 0);

DELETE FROM `scenario_data` WHERE `MapID` = 1812 OR `ScenarioID` = 1423;
INSERT INTO `scenario_data` (`ScenarioID`, `MapID`, `DifficultyID`, `Team`, `Class`, `LfgDungeonID`) VALUES
(1423, 1812, 12, 67, 0, 1634);

DELETE FROM `lfg_entrances` WHERE `dungeonId` = 1634;
INSERT INTO `lfg_entrances`
(`dungeonId`, `name`, `position_x`, `position_y`, `position_z`, `orientation`) VALUES
(1634, 'The Nightborne', 1192.70, 3484.22, 1.74045, 1.92098);

-- Arluin 107253, including display 69991, is present in the 7.3.5 WDB and
-- already has two retail Suramar spawns. Reuse his second original position
-- (map 1220, phase 8522) in the phase-independent scenario copy.
UPDATE `creature_template`
SET `ScriptName` = 'npc_nightborne_unlock_arluin'
WHERE `entry` = 107253;

DELETE FROM `creature` WHERE `map` = 1812;
INSERT INTO `creature`
(`id`, `map`, `zoneId`, `areaId`, `spawnMask`, `phaseMask`, `PhaseId`, `modelid`, `equipment_id`,
 `position_x`, `position_y`, `position_z`, `orientation`, `spawntimesecs`, `spawndist`, `currentwaypoint`,
 `curhealth`, `curmana`, `MovementType`, `npcflag`, `npcflag2`, `unit_flags`, `dynamicflags`, `AiID`,
 `MovementID`, `MeleeID`, `isActive`, `skipClone`, `personal_size`, `isTeemingSpawn`, `unit_flags3`) VALUES
(107253, 1812, 7637, 8379, 0, 1, '', 0, 0,
 1196.21, 3485.52, 1.74045, 1.92098, 300, 0, 0,
 1039267, 0, 0, 1, 0, 768, 0, 0,
 0, 0, 1, 0, 0, 0, 0);

-- BroadcastText 111478 is Arluin's original English warning and retains the
-- localized client text/sound data instead of introducing invented dialogue.
DELETE FROM `creature_text` WHERE `CreatureID` = 107253 AND `GroupID` = 8;
INSERT INTO `creature_text`
(`CreatureID`, `GroupID`, `ID`, `Text`, `Type`, `Language`, `Probability`, `Emote`, `Duration`, `Sound`,
 `BroadcastTextID`, `MinTimer`, `MaxTimer`, `SpellID`, `comment`) VALUES
(107253, 8, 0, 'This is not a safe place to linger. Keep walking.', 12, 0, 100, 1, 0, 0,
 111478, 0, 0, 0, 'Arluin - The Nightborne scenario completion');
