-- Activate the Lion's Landing instance and its seven 7.3.5 Scenario.db2 stages.
-- NPC/template values are restored from the Legends of Azeroth Pandaria 5.4.8
-- world release; scenario/criteria IDs come from the local 7.3.5 client DB2 files.

CREATE TABLE IF NOT EXISTS `_backup_20260919_lions_landing_instance_template` LIKE `instance_template`;
INSERT IGNORE INTO `_backup_20260919_lions_landing_instance_template`
SELECT * FROM `instance_template` WHERE `map` = 1103;

CREATE TABLE IF NOT EXISTS `_backup_20260919_lions_landing_templates` LIKE `creature_template`;
INSERT IGNORE INTO `_backup_20260919_lions_landing_templates`
SELECT * FROM `creature_template`
WHERE `entry` IN (67309,67413,67430,67434,67504,67548,67551,67692,67814,67920,68294,68508,
                  68581,68639,68640,68668,68669,68685,68851,68870,68871,68883,68884,68885,68886);

CREATE TABLE IF NOT EXISTS `_backup_20260919_lions_landing_spellclick` LIKE `npc_spellclick_spells`;
INSERT IGNORE INTO `_backup_20260919_lions_landing_spellclick`
SELECT * FROM `npc_spellclick_spells` WHERE `npc_entry` IN (68884,68885,68886);

UPDATE `instance_template`
SET `script` = 'instance_lions_landing'
WHERE `map` = 1103;

UPDATE `creature_template`
SET `ScriptName` = 'npc_lions_landing_interaction',
    `npcflag` = `npcflag` | 1
WHERE `entry` IN (68581,68685,68851,68870,68871,68883);

UPDATE `creature_template`
SET `ScriptName` = 'npc_lions_landing_interaction',
    `npcflag` = `npcflag` | 16777216
WHERE `entry` IN (68884,68885,68886);

-- Restore the original hostile factions and attackable flags.  These templates
-- were neutral/untargetable in the incomplete Legion database even though the
-- imported map spawns are Horde attackers in this Alliance scenario.
UPDATE `creature_template`
SET `faction` = 14, `unit_flags` = 0, `unit_flags2` = 2048
WHERE `entry` IN (67309,67413,67504,67548,67692,67814,68294,68508);

UPDATE `creature_template`
SET `faction` = 83, `unit_flags` = 0, `unit_flags2` = 2048
WHERE `entry` IN (67430,67434,67551,67920,68639,68640,68668,68669);

DELETE FROM `npc_spellclick_spells` WHERE `npc_entry` IN (68884,68885,68886);
INSERT INTO `npc_spellclick_spells` (`npc_entry`,`spell_id`,`cast_flags`,`user_type`,`add_npc_flag`) VALUES
(68884,135655,1,0,1), -- Place Bombs Here!
(68885,135656,1,0,1), -- Place Boomsticks Here!
(68886,135654,1,0,1); -- Place Rockets Here!
