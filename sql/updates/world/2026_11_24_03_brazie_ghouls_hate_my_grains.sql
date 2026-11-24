-- Reuse the existing scoped plant and seed scripts for Ghouls Hate My Grains.
INSERT IGNORE INTO `npc_spellclick_spells`
    (`npc_entry`, `spell_id`, `cast_flags`, `user_type`, `add_npc_flag`)
VALUES (49560, 92339, 2, 0, 1);

INSERT IGNORE INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
    (91704, 'spell_brazie_plant_spitter'),
    (91710, 'spell_brazie_plant_spitter'),
    (92339, 'spell_brazie_collect_freezya_seeds');
