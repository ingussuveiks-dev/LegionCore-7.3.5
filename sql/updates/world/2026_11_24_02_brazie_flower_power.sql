-- Flower Power uses the same scoped board and the original plant and seed spells.
INSERT IGNORE INTO `npc_spellclick_spells`
    (`npc_entry`, `spell_id`, `cast_flags`, `user_type`, `add_npc_flag`)
VALUES (49604, 92434, 2, 0, 1);

INSERT IGNORE INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
    (91646, 'spell_brazie_plant_spitter'),
    (92434, 'spell_brazie_collect_freezya_seeds'),
    (92440, 'spell_brazie_plant_spitter'),
    (92441, 'spell_brazie_freezya_blast');
