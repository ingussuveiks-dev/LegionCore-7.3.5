-- Restore the Arms artifact traits whose behavior cannot be expressed by the
-- final 7.3.5 client data alone.

-- The core already contains the exact Rage-cost calculations for Tactician and
-- Soul of the Slaughter. Remove the redundant bindings added by earlier audit
-- passes so their proc rolls happen exactly once.
DELETE FROM `spell_script_names` WHERE `ScriptName` IN
('spell_warr_corrupted_blood_of_zakajz', 'spell_warr_soul_of_the_slaughter',
 'spell_warr_tactician');

INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(209567, 'spell_warr_corrupted_blood_of_zakajz');

-- The old Shattered Defenses buff (209706) has the correct one-charge proc
-- row, but its final 7.3.5 replacement (248625) was missing it. The spell's
-- own class mask limits consumption to Mortal Strike and Execute.
DELETE FROM `spell_proc` WHERE `spellId` = 248625;

INSERT INTO `spell_proc`
(`spellId`, `schoolMask`, `spellFamilyName`, `spellFamilyMask0`,
 `spellFamilyMask1`, `spellFamilyMask2`, `spellFamilyMask3`, `typeMask`,
 `spellTypeMask`, `spellPhaseMask`, `hitMask`, `attributesMask`,
 `ratePerMinute`, `chance`, `cooldown`, `charges`, `modcharges`)
VALUES
(248625, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1, 1);
