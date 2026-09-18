-- Kadrak's gossip option explicitly offers a replacement Secret Signal Powder.
-- Quest 13808 (Mission Improbable) identifies that quest-start item as 45710;
-- the seven-digit 1212331 in the SmartAI action is not a 7.3.5 client item.
CREATE TABLE IF NOT EXISTS `_backup_20260918_kadrak_signal_powder_item` LIKE `smart_scripts`;

INSERT IGNORE INTO `_backup_20260918_kadrak_signal_powder_item`
SELECT *
FROM `smart_scripts`
WHERE `entryorguid` = 33837
  AND `source_type` = 0
  AND `id` = 0
  AND `event_type` = 62
  AND `action_type` = 56
  AND `action_param1` = 1212331;

UPDATE `smart_scripts`
SET `action_param1` = 45710,
    `comment` = 'Kadrak - On Gossip Select - Give Secret Signal Powder'
WHERE `entryorguid` = 33837
  AND `source_type` = 0
  AND `id` = 0
  AND `event_type` = 62
  AND `action_type` = 56
  AND `action_param1` = 1212331;
