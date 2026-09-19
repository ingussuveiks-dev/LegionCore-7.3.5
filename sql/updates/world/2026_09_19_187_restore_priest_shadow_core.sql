-- Restore core 7.3.5 Shadow Priest spells and Xal'atath artifact mechanics.
DELETE FROM `spell_script_names`
WHERE `ScriptName` IN
(
    'spell_pri_mind_bomb',
    'spell_pri_void_eruption',
    'spell_pri_void_bolt',
    'spell_pri_sphere_of_insanity',
    'spell_pri_sphere_of_insanity_damage',
    'spell_pri_shadow_word_death'
);

INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(205369, 'spell_pri_mind_bomb'),
(228260, 'spell_pri_void_eruption'),
(234746, 'spell_pri_void_bolt'),
(194200, 'spell_pri_sphere_of_insanity'),
(194225, 'spell_pri_sphere_of_insanity_damage'),
(32379,  'spell_pri_shadow_word_death'),
(199911, 'spell_pri_shadow_word_death');

-- Call to the Void already has its 7.3.5 proc/RPPM and summon data. Restore
-- the summoned tendril AI so entry 98167 channels Mind Flay at the proc target.
UPDATE `creature_template`
SET `ScriptName` = 'npc_pri_void_tendril'
WHERE `entry` = 98167;
