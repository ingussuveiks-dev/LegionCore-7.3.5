-- Restore the scripted effect for the Antorus destruction warlock 4-piece bonus.
DELETE FROM `spell_script_names`
WHERE `spell_id` = 251855 AND `ScriptName` = 'spell_warl_t21_destruction_4p';

INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(251855, 'spell_warl_t21_destruction_4p');
