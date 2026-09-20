-- Restore the scripted effects for the Antorus monk Tier 21 bonuses.
DELETE FROM `spell_script_names`
WHERE (`spell_id` = 251830 AND `ScriptName` = 'spell_monk_t21_brewmaster_4p')
   OR (`spell_id` = 100784 AND `ScriptName` = 'spell_monk_t21_windwalker_4p');

INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(251830, 'spell_monk_t21_brewmaster_4p'),
(100784, 'spell_monk_t21_windwalker_4p');
