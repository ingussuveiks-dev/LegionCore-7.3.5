-- Archivist Mechaton's sequence uses exact delays. After its final warning it
-- waits two seconds, becomes attackable, then attacks 1.5 seconds later. The
-- unit-flag step lost one zero from its upper delay bound.
CREATE TABLE IF NOT EXISTS `_backup_20260918_archivist_mechaton_attack_timer` LIKE `smart_scripts`;

INSERT IGNORE INTO `_backup_20260918_archivist_mechaton_attack_timer`
SELECT *
FROM `smart_scripts`
WHERE `entryorguid` = 2977500
  AND `source_type` = 9
  AND `id` = 10
  AND `link` = 0;

UPDATE `smart_scripts`
SET `event_param2` = 2000,
    `comment` = 'Archivist Mechaton - After 2 Seconds - Remove Non-Attackable Flag'
WHERE `entryorguid` = 2977500
  AND `source_type` = 9
  AND `id` = 10
  AND `link` = 0
  AND `event_type` = 0
  AND `event_param1` = 2000
  AND `event_param2` = 200
  AND `action_type` = 19
  AND `action_param1` = 768
  AND `target_type` = 1;
