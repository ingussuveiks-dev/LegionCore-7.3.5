-- Necrosis (216974) is the one-charge modifier granted after Death Coil. The
-- client has no SpellAuraOptions row for this secondary aura, so its proc event
-- must be defined in the world DB. Restrict charge consumption to the DK spell
-- family/class masks used by Scourge Strike and its Clawing Shadows replacement.
CREATE TABLE IF NOT EXISTS `_backup_20260918_necrosis_proc`
LIKE `spell_proc`;

REPLACE INTO `_backup_20260918_necrosis_proc`
SELECT *
FROM `spell_proc`
WHERE `spellId` = 216974
  AND `typeMask` = 0
  AND `chance` = 0;

UPDATE `spell_proc`
SET `spellFamilyName` = 15,
    `spellFamilyMask0` = 0,
    `spellFamilyMask1` = 134217728,
    `spellFamilyMask2` = 128,
    `spellFamilyMask3` = 0,
    `typeMask` = 16,
    `spellPhaseMask` = 1,
    `chance` = 100
WHERE `spellId` = 216974
  AND `typeMask` = 0
  AND `chance` = 0;
