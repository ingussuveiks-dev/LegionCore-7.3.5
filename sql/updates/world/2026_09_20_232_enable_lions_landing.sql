-- Enable Lion's Landing (map 1103, scenario 184).
-- The existing 253 creature and 6 gameobject spawns match the read-only
-- Legends of Azeroth Pandaria 5.4.8 world data. Legion build 26972 DB2
-- confirms difficulty 12, Alliance team, and LFG dungeon 590.
START TRANSACTION;

INSERT INTO `instance_template` (`map`,`parent`,`script`,`allowMount`,`bonusChance`) VALUES
(1103,870,'instance_lions_landing',1,20)
ON DUPLICATE KEY UPDATE
    `parent`=VALUES(`parent`),
    `script`=VALUES(`script`),
    `allowMount`=VALUES(`allowMount`),
    `bonusChance`=VALUES(`bonusChance`);

-- Keep all stage interaction actors friendly to Alliance players and bind
-- the script which drives the seven client criteria trees.
UPDATE `creature_template`
SET `faction`=1802,
    `ScriptName`='npc_lions_landing_interaction',
    `npcflag`=`npcflag` | 1
WHERE `entry` IN (68581,68685,68851,68870,68871,68883);

-- The weapon markers are shared with Domination Point; both mirrored
-- scenario scripts deliberately use the same three SetData slots.
UPDATE `creature_template`
SET `ScriptName`='npc_lions_landing_interaction',
    `npcflag`=`npcflag` | 16777216
WHERE `entry` IN (68884,68885,68886);

DELETE FROM `npc_spellclick_spells`
WHERE `npc_entry` IN (68884,68885,68886);

INSERT INTO `npc_spellclick_spells`
(`npc_entry`,`spell_id`,`cast_flags`,`user_type`,`add_npc_flag`) VALUES
(68884,135655,1,0,1),
(68885,135656,1,0,1),
(68886,135654,1,0,1);

-- Restore attackable Horde forces used by the three court groups and the
-- four final defence waves.
UPDATE `creature_template`
SET `faction`=14, `unit_flags`=0, `unit_flags2`=2048
WHERE `entry` IN (67309,67413,67504,67548,67692,67814,68294,68508);

UPDATE `creature_template`
SET `faction`=83, `unit_flags`=0, `unit_flags2`=2048
WHERE `entry` IN (67430,67434,67551,67920,68639,68640,68668,68669);

DELETE FROM `scenario_data`
WHERE `MapID`=1103 OR `ScenarioID`=184;

INSERT INTO `scenario_data`
(`ScenarioID`,`MapID`,`DifficultyID`,`Team`,`Class`,`LfgDungeonID`) VALUES
(184,1103,12,469,0,590);

-- Enable only after the instance, actors, criteria and LFG link are complete.
DELETE FROM `disables`
WHERE `sourceType`=2 AND `entry`=1103;

COMMIT;
