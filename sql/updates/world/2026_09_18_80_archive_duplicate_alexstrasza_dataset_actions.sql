-- A later SQL merge duplicated both of Alexstrasza's SetData orientation
-- handlers with link values equal to their ids. The original link=0 handlers
-- are already present and active; the copies are rejected self-links.
CREATE TABLE IF NOT EXISTS `_backup_20260918_duplicate_alexstrasza_dataset_actions` LIKE `smart_scripts`;

INSERT IGNORE INTO `_backup_20260918_duplicate_alexstrasza_dataset_actions`
SELECT *
FROM `smart_scripts`
WHERE `entryorguid` = 26917
  AND `source_type` = 0
  AND ((`id` = 0 AND `link` = 1 AND `event_type` = 38
        AND `event_param1` = 0 AND `event_param2` = 1
        AND `action_type` = 66 AND `target_type` = 8)
    OR (`id` = 1 AND `link` = 1 AND `event_type` = 38
        AND `event_param1` = 0 AND `event_param2` = 2
        AND `action_type` = 66 AND `target_type` = 1));

DELETE FROM `smart_scripts`
WHERE `entryorguid` = 26917
  AND `source_type` = 0
  AND ((`id` = 0 AND `link` = 1 AND `event_type` = 38
        AND `event_param1` = 0 AND `event_param2` = 1
        AND `action_type` = 66 AND `target_type` = 8)
    OR (`id` = 1 AND `link` = 1 AND `event_type` = 38
        AND `event_param1` = 0 AND `event_param2` = 2
        AND `action_type` = 66 AND `target_type` = 1));
