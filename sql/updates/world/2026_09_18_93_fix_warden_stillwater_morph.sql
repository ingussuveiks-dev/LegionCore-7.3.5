-- Warden Stillwater's transformed creature template (48103) already resolves to
-- display 30993. SmartAI morph accepts either a creature entry or a raw model,
-- never both; retain the creature-template reference so its display remains data-driven.
CREATE TABLE IF NOT EXISTS `_backup_20260918_warden_stillwater_morph` LIKE `smart_scripts`;

INSERT IGNORE INTO `_backup_20260918_warden_stillwater_morph`
SELECT *
FROM `smart_scripts`
WHERE `entryorguid` = 48080
  AND `source_type` = 0
  AND `id` = 3
  AND `link` = 4;

UPDATE `smart_scripts`
SET `action_param2` = 0,
    `comment` = 'Warden Stillwater - At 1-50% Health - Morph to Creature 48103'
WHERE `entryorguid` = 48080
  AND `source_type` = 0
  AND `id` = 3
  AND `link` = 4
  AND `event_type` = 2
  AND `action_type` = 3
  AND `action_param1` = 48103
  AND `action_param2` = 30993
  AND `target_type` = 1;
