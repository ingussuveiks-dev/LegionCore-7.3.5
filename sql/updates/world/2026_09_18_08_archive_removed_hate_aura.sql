-- Spell 10095 (Hate to Zero) came from an older client spell set and does not
-- exist in the 7.3.5 (26972) Spell.db2.  The only same-name spells still loaded
-- by this client are instant dummy effects, not persistent template auras.
CREATE TABLE IF NOT EXISTS `_backup_20260918_removed_hate_aura`
LIKE `creature_template_addon`;

REPLACE INTO `_backup_20260918_removed_hate_aura`
SELECT *
FROM `creature_template_addon`
WHERE `entry` IN (18521, 20315)
  AND `auras` = '10095';

UPDATE `creature_template_addon`
SET `auras` = ''
WHERE `entry` IN (18521, 20315)
  AND `auras` = '10095';
