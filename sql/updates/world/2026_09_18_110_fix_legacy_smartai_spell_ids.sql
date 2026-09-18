-- Several old-world SmartAI rows still referenced spell IDs removed before 7.3.5.
-- Preserve the original rows before replacing them with equivalent spells that
-- exist in the Legion client and are already used for the same purpose here.
CREATE TABLE IF NOT EXISTS `_backup_20260918_legacy_smartai_spell_ids` LIKE `smart_scripts`;

INSERT IGNORE INTO `_backup_20260918_legacy_smartai_spell_ids`
SELECT *
FROM `smart_scripts`
WHERE (`entryorguid` = 1538 AND `source_type` = 0 AND `id` = 0 AND `action_type` = 11 AND `action_param1` = 21562)
   OR (`entryorguid` = 1910 AND `source_type` = 0 AND `id` = 0 AND `action_type` = 11 AND `action_param1` = 331)
   OR (`entryorguid` = 3244 AND `source_type` = 0 AND `id` = 1 AND `action_type` = 11 AND `action_param1` = 50285)
   OR (`entryorguid` = 3246 AND `source_type` = 0 AND `id` = 1 AND `action_type` = 11 AND `action_param1` = 50285)
   OR (`entryorguid` = 3380 AND `source_type` = 0 AND `id` = 0 AND `action_type` = 11 AND `action_param1` = 689)
   OR (`entryorguid` = 329601 AND `source_type` = 0 AND `id` = 14 AND `action_type` = 11 AND `action_param1` = 6673);

-- Scarlet Friar: use the Legion-valid NPC Power Word: Fortitude.
UPDATE `smart_scripts`
SET `action_param1` = 13864
WHERE `entryorguid` = 1538 AND `source_type` = 0 AND `id` = 0
  AND `action_type` = 11 AND `action_param1` = 21562;

-- Muad: use the Legion-valid NPC Healing Wave already used by old-world casters.
UPDATE `smart_scripts`
SET `action_param1` = 11986
WHERE `entryorguid` = 1910 AND `source_type` = 0 AND `id` = 0
  AND `action_type` = 11 AND `action_param1` = 331;

-- Greater/Fleeting Plainstrider: 7272 is the valid Dust Cloud creature spell.
UPDATE `smart_scripts`
SET `action_param1` = 7272
WHERE `entryorguid` IN (3244, 3246) AND `source_type` = 0 AND `id` = 1
  AND `action_type` = 11 AND `action_param1` = 50285;

-- Burning Blade Acolyte: use the valid NPC Drain Life rank used by comparable casters.
UPDATE `smart_scripts`
SET `action_param1` = 17173
WHERE `entryorguid` = 3380 AND `source_type` = 0 AND `id` = 0
  AND `action_type` = 11 AND `action_param1` = 689;

-- Orgrimmar Grunt: use the valid creature Battle Shout used throughout old-world SAI.
UPDATE `smart_scripts`
SET `action_param1` = 9128
WHERE `entryorguid` = 329601 AND `source_type` = 0 AND `id` = 14
  AND `action_type` = 11 AND `action_param1` = 6673;
