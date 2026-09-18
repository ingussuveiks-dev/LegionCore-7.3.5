-- The first row in each staggered emote cycle had its cycle duration placed in
-- initial-max instead of repeat-max. Restore a zero-delay first step and the
-- same 16/32 second repeat used by the remaining rows in each cycle.
CREATE TABLE IF NOT EXISTS `_backup_20260918_exodar_emote_cycle_timers` LIKE `smart_scripts`;

INSERT IGNORE INTO `_backup_20260918_exodar_emote_cycle_timers`
SELECT *
FROM `smart_scripts`
WHERE `source_type` = 0
  AND `id` = 0
  AND `link` = 0
  AND (
       (`entryorguid` = 128562 AND `event_param1` = 0 AND `event_param2` = 16000 AND `event_param3` = 16000 AND `event_param4` = 0)
    OR (`entryorguid` IN (128656, 128657) AND `event_param1` = 0 AND `event_param2` = 32000 AND `event_param3` = 32000 AND `event_param4` = 0)
  );

UPDATE `smart_scripts`
SET `event_param2` = 0,
    `event_param4` = 16000,
    `comment` = 'T\'paartos - Out of Combat - Play Emote 11 Every 16 Seconds'
WHERE `entryorguid` = 128562
  AND `source_type` = 0
  AND `id` = 0
  AND `link` = 0
  AND `event_type` = 60
  AND `event_param1` = 0
  AND `event_param2` = 16000
  AND `event_param3` = 16000
  AND `event_param4` = 0
  AND `action_type` = 5
  AND `action_param1` = 11;

UPDATE `smart_scripts`
SET `event_param2` = 0,
    `event_param4` = 32000,
    `comment` = 'Exodar Citizen - Out of Combat - Play First Emote Every 32 Seconds'
WHERE `entryorguid` IN (128656, 128657)
  AND `source_type` = 0
  AND `id` = 0
  AND `link` = 0
  AND `event_type` = 60
  AND `event_param1` = 0
  AND `event_param2` = 32000
  AND `event_param3` = 32000
  AND `event_param4` = 0
  AND `action_type` = 5;
