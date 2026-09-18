-- Quest 10861: a Malevolent Hatchling summoned by a Cursed Egg must attack
-- the event invoker. The row used CLOSEST_PLAYER with an invalid zero radius.
CREATE TABLE IF NOT EXISTS `_backup_20260918_malevolent_hatchling_target` LIKE `smart_scripts`;

INSERT IGNORE INTO `_backup_20260918_malevolent_hatchling_target`
SELECT *
FROM `smart_scripts`
WHERE `entryorguid` = 22337
  AND `source_type` = 0
  AND `id` = 2
  AND `link` = 0;

UPDATE `smart_scripts`
SET `target_type` = 7,
    `target_param1` = 0,
    `comment` = 'Malevolent Hatchling - Just Summoned - Attack Invoker'
WHERE `entryorguid` = 22337
  AND `source_type` = 0
  AND `id` = 2
  AND `link` = 0
  AND `event_type` = 54
  AND `action_type` = 49
  AND `target_type` = 21
  AND `target_param1` = 0;
