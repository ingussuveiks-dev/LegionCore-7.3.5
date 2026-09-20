-- Restore the scripted effects for the Antorus shaman Tier 21 bonuses.
DELETE FROM `spell_script_names`
WHERE (`spell_id` = 251758 AND `ScriptName` = 'spell_sha_t21_elemental_4p')
   OR (`spell_id` = 251765 AND `ScriptName` = 'spell_sha_t21_restoration_4p');

INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(251758, 'spell_sha_t21_elemental_4p'),
(251765, 'spell_sha_t21_restoration_4p');
