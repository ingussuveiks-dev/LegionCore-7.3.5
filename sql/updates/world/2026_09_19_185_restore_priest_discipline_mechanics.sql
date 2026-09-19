-- Restore missing 7.3.5 Discipline Priest mechanics. These scripts implement
-- mechanics carried by the 7.3.5 client data rather than disabling any spell.
DELETE FROM `spell_script_names`
WHERE `ScriptName` IN
(
    'spell_pri_shadow_covenant',
    'spell_pri_penance_heal',
    'spell_pri_penance_channel',
    'spell_pri_power_word_solace',
    'spell_pri_mana_leech',
    'spell_pri_pain_suppression'
);

INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(204065, 'spell_pri_shadow_covenant'),
(47750,  'spell_pri_penance_heal'),
(47757,  'spell_pri_penance_channel'),
(47758,  'spell_pri_penance_channel'),
(129250, 'spell_pri_power_word_solace'),
(28305,  'spell_pri_mana_leech'),
(123050, 'spell_pri_mana_leech'),
(33206,  'spell_pri_pain_suppression');

-- Shadowfiend's 7.3.5 Mana Leech passive was missing from its pet aura
-- data. Mindbender already carries its corresponding 123050 passive.
DELETE FROM `spell_pet_auras`
WHERE `petEntry` = 19668 AND `spellId` = 28305;

INSERT INTO `spell_pet_auras`
(`petEntry`, `spellId`, `option`, `target`, `targetaura`, `bp0`, `bp1`, `bp2`, `aura`, `casteraura`, `createdspell`, `fromspell`, `comment`) VALUES
(19668, 28305, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 'Shadowfiend - Mana Leech (7.3.5 passive)');

-- Tyranny of Pain must receive every damage-taken event while Pain
-- Suppression is active. Its script applies the artifact trait's exact 33%.
DELETE FROM `spell_proc` WHERE `spellId` = 33206;
INSERT INTO `spell_proc`
(`spellId`, `schoolMask`, `spellFamilyName`, `spellFamilyMask0`, `spellFamilyMask1`, `spellFamilyMask2`, `spellFamilyMask3`, `typeMask`, `spellTypeMask`, `spellPhaseMask`, `hitMask`, `attributesMask`, `ratePerMinute`, `chance`, `cooldown`, `charges`, `modcharges`) VALUES
(33206, 0, 0, 0, 0, 0, 0, 0x00100000, 0, 0, 0, 0, 0, 100, 0, 0, 0);
