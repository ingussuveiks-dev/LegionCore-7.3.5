-- Three ambient Halls of Valor NPC scripts have a valid initial delay but a
-- zero upper bound. SmartAI rejects these UPDATE events because max < min.
-- Preserve the authored deterministic delay by matching max to min.
CREATE TABLE IF NOT EXISTS `_backup_20260918_halls_of_valor_emote_timers` LIKE `smart_scripts`;

INSERT IGNORE INTO `_backup_20260918_halls_of_valor_emote_timers`
SELECT *
FROM `smart_scripts`
WHERE `entryorguid` IN (95834, 95842, 97087)
  AND `source_type` = 0
  AND `id` = 2
  AND `link` = 0;

UPDATE `smart_scripts`
SET `event_param2` = `event_param1`
WHERE `entryorguid` IN (95834, 95842, 97087)
  AND `source_type` = 0
  AND `id` = 2
  AND `link` = 0
  AND `event_type` = 60
  AND `action_type` = 10
  AND `event_param1` > 0
  AND `event_param2` = 0;

-- NPC 97087 also has its repeat upper bound below the lower bound
-- (5000..4300 ms). Keep the authored five-second lower bound deterministic.
UPDATE `smart_scripts`
SET `event_param4` = `event_param3`
WHERE `entryorguid` = 97087
  AND `source_type` = 0
  AND `id` = 2
  AND `link` = 0
  AND `event_type` = 60
  AND `action_type` = 10
  AND `event_param3` = 5000
  AND `event_param4` = 4300;
