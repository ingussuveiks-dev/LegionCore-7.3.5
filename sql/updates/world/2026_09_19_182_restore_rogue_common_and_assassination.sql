-- Restore missing Rogue common, talent, and Assassination artifact mechanics.
DELETE FROM `spell_script_names`
WHERE (`spell_id` = 703 AND `ScriptName` = 'spell_rog_garrote')
   OR (`spell_id` = 408 AND `ScriptName` = 'spell_rog_internal_bleeding')
   OR (`spell_id` = 36554 AND `ScriptName` = 'spell_rog_shadow_swiftness')
   OR (`spell_id` = 137619 AND `ScriptName` = 'spell_rog_marked_for_death')
   OR (`spell_id` = 192424 AND `ScriptName` = 'spell_rog_surge_of_toxins')
   OR (`spell_id` = 193539 AND `ScriptName` = 'spell_rog_alacrity')
   OR (`spell_id` = 238138 AND `ScriptName` = 'spell_rog_sinister_circulation');

INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(703, 'spell_rog_garrote'),
(408, 'spell_rog_internal_bleeding'),
(36554, 'spell_rog_shadow_swiftness'),
(137619, 'spell_rog_marked_for_death'),
(192424, 'spell_rog_surge_of_toxins'),
(193539, 'spell_rog_alacrity'),
(238138, 'spell_rog_sinister_circulation');
