-- Restore Legion 7.3.5 Mistweaver mechanics missing from the world data.

DELETE FROM `spell_script_names`
WHERE (`spell_id` = 116670 AND `ScriptName` = 'spell_monk_vivify')
   OR (`spell_id` = 115310 AND `ScriptName` = 'spell_monk_revival')
   OR (`spell_id` = 191840 AND `ScriptName` = 'spell_monk_essence_font_heal')
   OR (`spell_id` = 193884 AND `ScriptName` = 'spell_monk_soothing_mist_passive')
   OR (`spell_id` = 197915 AND `ScriptName` = 'spell_monk_lifecycles')
   OR (`spell_id` = 199384 AND `ScriptName` = 'spell_monk_spirit_tether')
   OR (`spell_id` = 199640 AND `ScriptName` = 'spell_monk_celestial_breath_driver')
   OR (`spell_id` = 199656 AND `ScriptName` = 'spell_monk_celestial_breath')
   OR (`spell_id` = 214483 AND `ScriptName` = 'spell_monk_sheiluns_clouds')
   OR (`spell_id` = 242400 AND `ScriptName` = 'spell_monk_whispers_of_shaohao');

INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(115310, 'spell_monk_revival'),
(116670, 'spell_monk_vivify'),
(191840, 'spell_monk_essence_font_heal'),
(193884, 'spell_monk_soothing_mist_passive'),
(197915, 'spell_monk_lifecycles'),
(199384, 'spell_monk_spirit_tether'),
(199640, 'spell_monk_celestial_breath_driver'),
(199656, 'spell_monk_celestial_breath'),
(214483, 'spell_monk_sheiluns_clouds'),
(242400, 'spell_monk_whispers_of_shaohao');

-- Entry 101297 is the six-second visual spirit summoned by Blessings of
-- Yu'lon. The unrelated Antorus "Spore Filled" spell made it act like an
-- encounter creature instead; the actual artifact healing is restored above.
DELETE FROM `creature_template_spell`
WHERE `entry` = 101297 AND `spell` = 252113;
