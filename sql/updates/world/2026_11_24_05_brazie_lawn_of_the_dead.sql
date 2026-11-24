-- Bind the reused fertilizer actor and its native click.
INSERT IGNORE INTO `npc_spellclick_spells`
    (`npc_entry`, `spell_id`, `cast_flags`, `user_type`, `add_npc_flag`)
VALUES (50067, 93100, 2, 0, 1);

INSERT IGNORE INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
    (93100, 'spell_brazie_collect_fertilizer');
