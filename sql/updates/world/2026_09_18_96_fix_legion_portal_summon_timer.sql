-- Every summon in the Legion Portal timed action list has an exact ten-second
-- delay. The third Greater Imp row had a truncated one-second maximum.
CREATE TABLE IF NOT EXISTS `_backup_20260918_legion_portal_summon_timer` LIKE `smart_scripts`;

INSERT IGNORE INTO `_backup_20260918_legion_portal_summon_timer`
SELECT *
FROM `smart_scripts`
WHERE `entryorguid` = 111357
  AND `source_type` = 9
  AND `id` = 2
  AND `link` = 0;

UPDATE `smart_scripts`
SET `event_param2` = 10000,
    `comment` = 'Legion Portal - After 10 Seconds - Summon Greater Imp'
WHERE `entryorguid` = 111357
  AND `source_type` = 9
  AND `id` = 2
  AND `link` = 0
  AND `event_type` = 0
  AND `event_param1` = 10000
  AND `event_param2` = 1000
  AND `action_type` = 12
  AND `action_param1` = 111424
  AND `action_param2` = 8
  AND `target_type` = 1;
