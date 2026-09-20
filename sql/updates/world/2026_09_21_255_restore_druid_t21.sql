-- Restore the scripted effects for the Antorus druid Tier 21 bonuses.
DELETE FROM `spell_script_names`
WHERE (`spell_id` = 251789 AND `ScriptName` = 'spell_dru_t21_feral_2p')
   OR (`spell_id` = 33917 AND `ScriptName` = 'spell_dru_t21_guardian_2p')
   OR (`spell_id` = 22812 AND `ScriptName` = 'spell_dru_t21_guardian_4p');

INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(251789, 'spell_dru_t21_feral_2p'),
(33917, 'spell_dru_t21_guardian_2p'),
(22812, 'spell_dru_t21_guardian_4p');
