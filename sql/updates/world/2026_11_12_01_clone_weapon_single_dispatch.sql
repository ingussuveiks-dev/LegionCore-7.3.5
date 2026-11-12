-- Native 69893 is already handled by the shared paired-slot clone aura.
-- The extra Barim handler snapshots the already copied weapon and restores it last.
DELETE FROM `spell_script_names` WHERE `spell_id` = 69893 AND `ScriptName` = 'spell_copy_melee_weapon';
