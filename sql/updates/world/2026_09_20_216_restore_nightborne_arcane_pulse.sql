-- Arcane Pulse has no client-side damage coefficient in 7.3.5. Restore the
-- server-side Nightborne racial scaling used by the Legion implementation.
DELETE FROM `spell_script_names`
WHERE `spell_id` = 260364 AND `ScriptName` = 'spell_arcane_pulse';

INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(260364, 'spell_arcane_pulse');
