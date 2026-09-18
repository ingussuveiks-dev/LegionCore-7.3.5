-- Area trigger 6854 runs two independent ON_TRIGGER actions. The second one
-- incorrectly linked to itself even though the first action starts no chain.
CREATE TABLE IF NOT EXISTS `_backup_20260918_areatrigger_6854_aura_action` LIKE `smart_scripts`;

INSERT IGNORE INTO `_backup_20260918_areatrigger_6854_aura_action`
SELECT *
FROM `smart_scripts`
WHERE `entryorguid` = 6854
  AND `source_type` = 2
  AND `id` IN (0, 1);

UPDATE `smart_scripts`
SET `link` = 0
WHERE `entryorguid` = 6854
  AND `source_type` = 2
  AND `id` = 1
  AND `link` = 1
  AND `event_type` = 46
  AND `event_param1` = 6854
  AND `action_type` = 28
  AND `action_param1` = 99435
  AND `target_type` = 7;
