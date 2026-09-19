-- Shadowy Apparitions (78203) may only proc from critical Shadow Word: Pain
-- periodic damage.  Keeping the spell-family filter in the database lets the
-- core's generic proc system retain the original 7.3.5 trigger behaviour.
UPDATE `spell_proc_event`
SET `SpellFamilyName` = 6,
    `SpellFamilyMask0` = 32768,
    `SpellFamilyMask1` = 0,
    `SpellFamilyMask2` = 0,
    `SpellFamilyMask3` = 0
WHERE `entry` = 78203;
