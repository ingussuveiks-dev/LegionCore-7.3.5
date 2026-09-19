-- Restore Legion 7.3.5 Feral artifact proc restrictions.

-- Shadow Thrash (210676) has a native 25% proc chance and trigger spell, but
-- without its family mask it can proc from unrelated druid abilities.  It is
-- only triggered by the Feral Thrash spell family flag.
DELETE FROM `spell_proc_event` WHERE `entry` = 210676;

INSERT INTO `spell_proc_event`
(`entry`, `SchoolMask`, `SpellFamilyName`, `SpellFamilyMask0`, `SpellFamilyMask1`,
 `SpellFamilyMask2`, `SpellFamilyMask3`, `procFlags`, `procEx`, `ppmRate`,
 `CustomChance`, `Cooldown`, `effectmask`)
VALUES
(210676, 0, 7, 0, 0, 4194304, 0, 0, 0, 0, 0, 0, 1);

-- Predator was hotfixed during Legion so a target dying while affected by
-- Ashamane's Frenzy also resets Tiger's Fury.  Rip is handled in C++, while
-- Rake and Thrash already have equivalent death-removal links in the world DB.
DELETE FROM `spell_linked_spell`
WHERE `spell_trigger` = -210722 AND `spell_effect` = 5217 AND `type` = 0;

INSERT INTO `spell_linked_spell`
(`spell_trigger`, `spell_effect`, `type`, `caster`, `target`, `hastype`,
 `hastalent`, `hasparam`, `hastype2`, `hastalent2`, `hasparam2`, `chance`,
 `cooldown`, `duration`, `hitmask`, `removeMask`, `effectMask`,
 `targetCountType`, `targetCount`, `actiontype`, `group`, `param`, `randList`,
 `comment`)
VALUES
(-210722, 5217, 0, 3, 3, 1, 202021, 0, 0, 0, 0, 0, 0, 0, 0, 32, 0,
 0, -1, 8, 0, 0, '', 'Predator: Ashamane''s Frenzy death resets Tiger''s Fury');
