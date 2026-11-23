-- The tutorial uses the original Solar Power vehicle and one native plant spell.
UPDATE `creature_template` SET `ScriptName` = 'npc_brazie_basic_botany' WHERE `entry` = 49687;
UPDATE `creature_template` SET `ScriptName` = 'npc_brazie_basic_botany_board', `VehicleId` = 1382, `spell1` = 91649 WHERE `entry` = 49176;

INSERT IGNORE INTO `npc_spellclick_spells`
    (`npc_entry`, `spell_id`, `cast_flags`, `user_type`, `add_npc_flag`)
VALUES (49194, 91687, 2, 0, 1);

INSERT IGNORE INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
    (91649, 'spell_brazie_plant_spitter'),
    (91687, 'spell_brazie_collect_solar_power'),
    (91748, 'spell_brazie_spitter_venom');
