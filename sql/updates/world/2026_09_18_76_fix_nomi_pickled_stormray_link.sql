-- Nomi's first Pickled Stormray recipe exchange stopped after removing the
-- fish because linked action 18 pointed to itself. The duplicate exchange on
-- the second gossip page confirms that action 18 must continue to action 19.
CREATE TABLE IF NOT EXISTS `_backup_20260918_nomi_pickled_stormray_link` LIKE `smart_scripts`;

INSERT IGNORE INTO `_backup_20260918_nomi_pickled_stormray_link`
SELECT *
FROM `smart_scripts`
WHERE `entryorguid` = 101846
  AND `source_type` = 0
  AND `id` IN (18, 19);

UPDATE `smart_scripts`
SET `link` = 19
WHERE `entryorguid` = 101846
  AND `source_type` = 0
  AND `id` = 18
  AND `link` = 18
  AND `event_type` = 61
  AND `action_type` = 57
  AND `action_param1` = 124110
  AND `action_param2` = 5;
