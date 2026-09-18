-- Gallywix's opening casts are staggered into 1-3, 4-5.8, 6-7, and 8-10
-- second windows. You're Fired lost one zero from the 10-second upper bound.
CREATE TABLE IF NOT EXISTS `_backup_20260918_gallywix_youre_fired_timer` LIKE `smart_scripts`;

INSERT IGNORE INTO `_backup_20260918_gallywix_youre_fired_timer`
SELECT *
FROM `smart_scripts`
WHERE `entryorguid` = 395820
  AND `source_type` = 0
  AND `id` = 4
  AND `link` = 5;

UPDATE `smart_scripts`
SET `event_param2` = 10000,
    `comment` = 'Trade Prince Gallywix - In Combat - Cast You\'re Fired (8-10 Second Initial)'
WHERE `entryorguid` = 395820
  AND `source_type` = 0
  AND `id` = 4
  AND `link` = 5
  AND `event_type` = 0
  AND `event_param1` = 8000
  AND `event_param2` = 1000
  AND `event_param3` = 17000
  AND `event_param4` = 20000
  AND `action_type` = 11
  AND `action_param1` = 74004
  AND `target_type` = 2;
