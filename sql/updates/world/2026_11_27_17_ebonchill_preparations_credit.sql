-- Complete the arrival conversation through its original timed list.
-- The waypoint still provides the ward casting animation.
START TRANSACTION;

SET @meryl_los_regression = (
    SELECT COUNT(*) = 4 FROM `smart_scripts`
    WHERE `entryorguid` = 107976 AND `source_type` = 0 AND (
        (`id` = 3 AND `link` = 4 AND `event_type` = 61 AND `action_type` = 27 AND `action_param1` = 22870) OR
        (`id` = 4 AND `action_type` = 80 AND `action_param1` = 10797600) OR
        (`id` = 5 AND `event_type` = 10 AND `event_phase_mask` = 1 AND `action_type` = 205 AND `action_param2` = 51381) OR
        (`id` = 7 AND `action_type` = 80 AND `action_param1` = 10797601))
) AND NOT EXISTS (
    SELECT 1 FROM `smart_scripts` WHERE `entryorguid` = 107976 AND `source_type` = 9 AND `id` BETWEEN 2 AND 5
);

UPDATE `smart_scripts` SET `link` = 0
WHERE @meryl_los_regression AND `entryorguid` = 107976 AND `source_type` = 0 AND `id` = 3 AND `link` = 4;

DELETE FROM `smart_scripts`
WHERE @meryl_los_regression AND (
    (`entryorguid` = 107976 AND `source_type` = 0 AND `id` BETWEEN 4 AND 7) OR
    (`entryorguid` IN (10797600, 10797601) AND `source_type` = 9));

DELETE FROM `conditions`
WHERE @meryl_los_regression AND `SourceTypeOrReferenceId` = 22 AND `SourceGroup` = 6
  AND `SourceEntry` = 107976 AND `SourceId` = 0 AND `ElseGroup` = 0;

INSERT INTO `smart_scripts`
    (`entryorguid`, `source_type`, `id`, `event_type`, `event_flags`, `event_param1`, `event_param2`,
     `action_type`, `action_param1`, `action_param2`, `target_type`, `target_param1`, `comment`)
SELECT 107976, 9, 2, 0, 1, 27000, 27000, 205, 92, 51381, 21, 50, 'Meryl completes Preparations for a nearby player'
WHERE @meryl_los_regression
UNION ALL SELECT 107976, 9, 3, 0, 1, 0, 0, 5, 66, 0, 1, 0, 'Meryl salutes before leaving'
WHERE @meryl_los_regression
UNION ALL SELECT 107976, 9, 4, 0, 1, 1200, 1200, 11, 195453, 2, 1, 0, 'Meryl plays the original teleport visual'
WHERE @meryl_los_regression
UNION ALL SELECT 107976, 9, 5, 0, 1, 500, 500, 41, 0, 0, 1, 0, 'Remove introductory Meryl after the teleport visual'
WHERE @meryl_los_regression;

COMMIT;
