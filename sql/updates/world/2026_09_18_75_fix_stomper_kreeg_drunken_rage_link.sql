-- Stomper Kreeg's 15% health Drunken Rage action incorrectly linked to
-- itself. The immediately following LINK event (id 9) is its intended action.
CREATE TABLE IF NOT EXISTS `_backup_20260918_stomper_kreeg_rage_link` LIKE `smart_scripts`;

INSERT IGNORE INTO `_backup_20260918_stomper_kreeg_rage_link`
SELECT *
FROM `smart_scripts`
WHERE `entryorguid` = 14322
  AND `source_type` = 0
  AND `id` IN (8, 9);

UPDATE `smart_scripts`
SET `link` = 9
WHERE `entryorguid` = 14322
  AND `source_type` = 0
  AND `id` = 8
  AND `link` = 8
  AND `event_type` = 2
  AND `event_param1` = 0
  AND `event_param2` = 15
  AND `action_type` = 11
  AND `action_param1` = 22835;
