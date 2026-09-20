-- Restore the scripted Vengeance effect for the Antorus demon hunter Tier 21 set.
-- Havoc 4P is handled by the existing Eye Beam script.
DELETE FROM `spell_script_names`
WHERE `spell_id` = 251774 AND `ScriptName` = 'spell_dh_t21_vengeance_4p';

INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(251774, 'spell_dh_t21_vengeance_4p');
