-- These charge-limited auras modify the next eligible spell, but their custom
-- spell_proc rows omit the required cast phase. Their proc type and chance are
-- supplied by 7.3.5 SpellAuraOptions; only the missing phase is overridden.
-- CAST consumes the charge once per eligible cast instead of once per hit or
-- target, matching each aura's "next spell" wording.
CREATE TABLE IF NOT EXISTS `_backup_20260918_next_spell_proc_phase`
LIKE `spell_proc`;

REPLACE INTO `_backup_20260918_next_spell_proc_phase`
SELECT *
FROM `spell_proc`
WHERE `spellId` IN
      (164545, 164547, 198300, 206333, 208081, 209493, 209706, 213708)
  AND `spellPhaseMask` = 0;

UPDATE `spell_proc`
SET `spellPhaseMask` = 1
WHERE `spellId` IN
      (164545, 164547, 198300, 206333, 208081, 209493, 209706, 213708)
  AND `spellPhaseMask` = 0;
