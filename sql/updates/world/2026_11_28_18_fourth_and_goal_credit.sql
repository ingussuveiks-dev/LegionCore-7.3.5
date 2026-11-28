-- One Footbomb credit per hit. The original LINK keeps its summon parameters
-- and becomes a separate, once-per-reset reaction to the same spell.
START TRANSACTION;

SET @fourth_and_goal_original := (
    SELECT COUNT(*) = 3
       AND SUM((`id`, `link`, `event_type`, `event_flags`, `event_param1`, `action_type`,
                `action_param1`, `action_param2`, `action_param3`, `target_type`) =
               (0, 0, 8, 0, 70052, 33, 37203, 0, 0, 7)) = 1
       AND SUM((`id`, `link`, `event_type`, `event_flags`, `event_param1`, `action_type`,
                `action_param1`, `action_param2`, `action_param3`, `target_type`) =
               (0, 1, 8, 1, 70052, 33, 37203, 0, 0, 7)) = 1
       AND SUM((`id`, `link`, `event_type`, `event_flags`, `event_param1`, `action_type`,
                `action_param1`, `action_param2`, `action_param3`, `target_type`) =
               (1, 0, 61, 1, 0, 12, 423070, 1, 300000, 8)) = 1
       AND SUM(`Difficulties` = '' AND `event_phase_mask` = 0 AND `event_chance` = 100
               AND (`event_param2`, `event_param3`, `event_param4`, `event_param5`,
                    `action_param4`, `action_param5`, `action_param6`,
                    `target_param1`, `target_param2`, `target_param3`, `target_param4`) =
                   (0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0)) = 3
    FROM `smart_scripts`
    WHERE `entryorguid` = 37203 AND `source_type` = 0
) AND NOT EXISTS (
    SELECT 1 FROM `conditions`
    WHERE `SourceTypeOrReferenceId` = 22 AND `SourceEntry` = 37203 AND `SourceId` = 0
);

UPDATE `smart_scripts`
SET `event_type` = 8, `event_param1` = 70052,
    `comment` = 'Fourth And Goal Target - On Footbomb Hit - Summon Scene Actor (No Repeat)'
WHERE @fourth_and_goal_original AND `entryorguid` = 37203 AND `source_type` = 0 AND `id` = 1 AND `link` = 0;

DELETE FROM `smart_scripts`
WHERE @fourth_and_goal_original AND `entryorguid` = 37203 AND `source_type` = 0 AND `id` = 0 AND `link` = 1;

COMMIT;
SET @fourth_and_goal_original := NULL;
