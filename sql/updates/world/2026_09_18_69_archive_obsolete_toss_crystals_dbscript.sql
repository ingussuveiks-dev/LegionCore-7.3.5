-- In client build 7.3.5.26972 spell 179915 (Toss Crystals) has REMOVE_AURA at
-- effect 0 and PERIODIC_TRIGGER_SPELL at effect 1. It has no SCRIPT_EFFECT or
-- DUMMY effect, so these seven identical legacy DB-script rows are unreachable.
CREATE TABLE IF NOT EXISTS `_backup_20260918_toss_crystals_spell_scripts` LIKE `spell_scripts`;

INSERT INTO `_backup_20260918_toss_crystals_spell_scripts`
SELECT *
FROM `spell_scripts`
WHERE `id` = 179915
  AND `effIndex` = 0
  AND `command` = 8
  AND `datalong` = 90315;

DELETE FROM `spell_scripts`
WHERE `id` = 179915
  AND `effIndex` = 0
  AND `command` = 8
  AND `datalong` = 90315;
