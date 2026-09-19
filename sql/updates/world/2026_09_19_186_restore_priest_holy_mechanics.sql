-- Restore 7.3.5 Holy Priest targeting, Holy Word, and artifact mechanics.
DELETE FROM `spell_script_names`
WHERE `ScriptName` IN
(
    'spell_pri_holy_word_chastise',
    'spell_pri_serendipity',
    'spell_pri_binding_heal',
    'spell_pri_prayer_of_healing_targets',
    'spell_pri_circle_of_healing',
    'spell_pri_prayer_of_mending',
    'spell_pri_holy_mending',
    'spell_pri_cosmic_ripple',
    'spell_pri_cosmic_ripple_cooldown'
);

INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(88625,  'spell_pri_holy_word_chastise'),
(63733,  'spell_pri_serendipity'),
(32546,  'spell_pri_binding_heal'),
(596,    'spell_pri_prayer_of_healing_targets'),
(204883, 'spell_pri_circle_of_healing'),
(33076,  'spell_pri_prayer_of_mending'),
(196779, 'spell_pri_holy_mending'),
(243241, 'spell_pri_cosmic_ripple'),
(243283, 'spell_pri_cosmic_ripple_cooldown');

-- The old generic Serendipity mapping let unrelated effects modify several
-- Holy Words at once. Restrict the proc to its five 7.3.5 source spells; the
-- AuraScript routes each source to its proper cooldown and applies talents.
DELETE FROM `spell_proc_event` WHERE `entry` = 63733;
DELETE FROM `spell_proc` WHERE `spellId` = 63733;
DELETE FROM `spell_trigger` WHERE `spell_id` = 63733;

INSERT INTO `spell_proc`
(`spellId`, `schoolMask`, `spellFamilyName`, `spellFamilyMask0`, `spellFamilyMask1`, `spellFamilyMask2`, `spellFamilyMask3`, `typeMask`, `spellTypeMask`, `spellPhaseMask`, `hitMask`, `attributesMask`, `ratePerMinute`, `chance`, `cooldown`, `charges`, `modcharges`) VALUES
(63733, 0, 6, 0x00001AA0, 0x00000004, 0, 0, 0, 7, 2, 0, 0, 0, 100, 0, 0, 0);

-- Holy Mending is conditional on the jump target carrying this priest's Renew.
-- Its C++ script performs the caster-specific check instead of healing every jump.
DELETE FROM `spell_trigger` WHERE `spell_id` = 196779;

-- Focus in the Light only activates while Focused Will is active.
UPDATE `spell_trigger`
SET `aura` = 45242
WHERE `spell_id` = 196419 AND `spell_trigger` IN (210979, 210980);
