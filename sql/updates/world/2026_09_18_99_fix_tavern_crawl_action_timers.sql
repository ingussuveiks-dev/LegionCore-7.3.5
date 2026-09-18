-- Four Kirin Tor Tavern Crawl ambient sequences contain reversed delay bounds.
-- Preserve every authored duration and only normalize each invalid min/max pair.
CREATE TABLE IF NOT EXISTS `_backup_20260918_tavern_crawl_action_timers` LIKE `smart_scripts`;

INSERT IGNORE INTO `_backup_20260918_tavern_crawl_action_timers`
SELECT *
FROM `smart_scripts`
WHERE `source_type` = 9
  AND `event_type` = 0
  AND `event_param1` > `event_param2`
  AND `entryorguid` IN (11005194, 11005224, 11005239, 11005273);

UPDATE `smart_scripts` AS `active`
INNER JOIN `_backup_20260918_tavern_crawl_action_timers` AS `original`
        ON `original`.`entryorguid` = `active`.`entryorguid`
       AND `original`.`source_type` = `active`.`source_type`
       AND `original`.`id` = `active`.`id`
       AND `original`.`link` = `active`.`link`
SET `active`.`event_param1` = `original`.`event_param2`,
    `active`.`event_param2` = `original`.`event_param1`
WHERE `active`.`source_type` = 9
  AND `active`.`event_type` = 0
  AND `active`.`event_param1` > `active`.`event_param2`
  AND `active`.`entryorguid` IN (11005194, 11005224, 11005239, 11005273);
