-- Restore the scripted effects for the Antorus death knight Tier 21 bonuses.
DELETE FROM `spell_script_names`
WHERE (`spell_id` = 251871 AND `ScriptName` = 'spell_dk_t21_unholy_2p')
   OR (`spell_id` = 251872 AND `ScriptName` = 'spell_dk_t21_unholy_4p')
   OR (`spell_id` = 49028 AND `ScriptName` = 'spell_dk_t21_blood_4p');

INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(251871, 'spell_dk_t21_unholy_2p'),
(251872, 'spell_dk_t21_unholy_4p'),
(49028, 'spell_dk_t21_blood_4p');
