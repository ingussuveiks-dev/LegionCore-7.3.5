-- Agony procs from the Affliction T21 2P bonus extend the caster's active
-- Unstable Affliction applications on the proc target by the DB2 amount.
DELETE FROM `spell_script_names`
WHERE `spell_id` = 251847 AND `ScriptName` = 'spell_warl_t21_affliction_2p';

INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(251847, 'spell_warl_t21_affliction_2p');
