-- Data-set event 2 already starts Gul'dan's waypoint and casts 184538. A third
-- row for the same event is an unconfigured CAST action with spell 0, so retain
-- it for provenance and remove only that inert imported placeholder.
CREATE TABLE IF NOT EXISTS `_backup_20260918_guldan_incomplete_spell_action` LIKE `smart_scripts`;

INSERT IGNORE INTO `_backup_20260918_guldan_incomplete_spell_action`
SELECT *
FROM `smart_scripts`
WHERE `entryorguid` = 93926 AND `source_type` = 0 AND `id` = 1
  AND `event_type` = 38 AND `event_param1` = 2 AND `event_param2` = 2
  AND `action_type` = 11 AND `action_param1` = 0;

DELETE FROM `smart_scripts`
WHERE `entryorguid` = 93926 AND `source_type` = 0 AND `id` = 1
  AND `event_type` = 38 AND `event_param1` = 2 AND `event_param2` = 2
  AND `action_type` = 11 AND `action_param1` = 0;
