-- Summoning Portal's periodic spell had a 120000..12000 ms repeat window.
-- The upper bound lost one zero; keep the intended deterministic two minutes.
CREATE TABLE IF NOT EXISTS `_backup_20260918_summoning_portal_timer` LIKE `smart_scripts`;

INSERT IGNORE INTO `_backup_20260918_summoning_portal_timer`
SELECT *
FROM `smart_scripts`
WHERE `entryorguid` = 105038
  AND `source_type` = 0
  AND `id` = 0
  AND `link` = 0;

UPDATE `smart_scripts`
SET `event_param4` = 120000,
    `comment` = 'Summoning Portal - Every 120 Seconds - Cast Spell 208041'
WHERE `entryorguid` = 105038
  AND `source_type` = 0
  AND `id` = 0
  AND `link` = 0
  AND `event_type` = 60
  AND `event_param3` = 120000
  AND `event_param4` = 12000
  AND `action_type` = 11
  AND `action_param1` = 208041;
