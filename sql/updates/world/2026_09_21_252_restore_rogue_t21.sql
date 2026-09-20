-- Restore the scripted effects for the Antorus rogue Tier 21 bonuses.
DELETE FROM `spell_script_names`
WHERE (`spell_id` = 251785 AND `ScriptName` = 'spell_rog_t21_subtlety_2p')
   OR (`spell_id` = 257945 AND `ScriptName` = 'spell_rog_t21_shadow_gestures');

INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(251785, 'spell_rog_t21_subtlety_2p'),
(257945, 'spell_rog_t21_shadow_gestures');
