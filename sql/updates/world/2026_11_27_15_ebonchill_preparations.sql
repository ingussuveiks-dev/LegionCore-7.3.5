-- Meryl's arrival conversation, real waypoint, ward preparation and departure.
START TRANSACTION;

SET @original_intro = (
    SELECT COUNT(*) = 4 FROM `smart_scripts`
    WHERE `entryorguid` = 107976 AND `source_type` = 9 AND (
        (`id` = 0 AND `event_param1` = 20000 AND `action_type` = 53 AND `action_param2` = 107976) OR
        (`id` = 1 AND `event_param1` = 27000 AND `action_type` = 205 AND `action_param2` = 51381) OR
        (`id` = 2 AND `action_type` = 11 AND `action_param1` = 195453) OR
        (`id` = 3 AND `action_type` = 41))
) AND EXISTS (
    SELECT 1 FROM `creature` WHERE `guid` = 369759 AND `id` = 107976 AND `map` = 1616
) AND EXISTS (
    SELECT 1 FROM `waypoints` WHERE `entry` = 107976 AND `pointid` = 1
) AND NOT EXISTS (
    SELECT 1 FROM `smart_scripts`
    WHERE (`entryorguid` = 107976 AND `source_type` = 0 AND `id` BETWEEN 2 AND 7)
       OR (`entryorguid` IN (10797600, 10797601) AND `source_type` = 9)
);

UPDATE `smart_scripts`
SET `action_type` = 5, `action_param1` = 2, `action_param2` = 0,
    `comment` = 'Meryl bows during the arrival conversation'
WHERE @original_intro AND `entryorguid` = 107976 AND `source_type` = 9 AND `id` = 0;

UPDATE `smart_scripts`
SET `event_param1` = 900, `event_param2` = 900, `action_type` = 53, `action_param1` = 0,
    `action_param2` = 107976, `target_type` = 1, `target_param1` = 0,
    `comment` = 'Meryl resumes the original walk after bowing'
WHERE @original_intro AND `entryorguid` = 107976 AND `source_type` = 9 AND `id` = 1;

DELETE FROM `smart_scripts`
WHERE @original_intro AND `entryorguid` = 107976 AND `source_type` = 9 AND `id` IN (2, 3);

INSERT INTO `smart_scripts`
    (`entryorguid`, `source_type`, `id`, `link`, `event_type`, `event_phase_mask`, `event_flags`,
     `event_param1`, `event_param2`, `action_type`, `action_param1`, `action_param2`, `action_param3`,
     `target_type`, `comment`)
SELECT rows_to_add.* FROM (
    SELECT 107976 AS `entry`, 0 AS `source`, 2 AS `event_id`, 3 AS `link_id`, 40 AS `event`,
        0 AS `phase_mask`, 1 AS `flags`, 1 AS `event_p1`, 107976 AS `event_p2`, 5 AS `action`,
        51 AS `action_p1`, 0 AS `action_p2`, 0 AS `action_p3`, 1 AS `target`,
        'Meryl casts when he reaches the original endpoint' AS `description`
    UNION ALL SELECT 107976, 0, 3, 4, 61, 0, 1, 0, 0, 27, 22870, 0, 1500, 7, 'Show the ward magic to the player listening to Meryl'
    UNION ALL SELECT 107976, 0, 4, 0, 61, 0, 1, 0, 0, 80, 10797600, 2, 0, 1, 'Let the ward instructions finish after the cast'
    UNION ALL SELECT 107976, 0, 5, 6, 10, 1, 1, 1, 45, 205, 92, 51381, 0, 7, 'Complete Preparations for a nearby eligible player'
    UNION ALL SELECT 107976, 0, 6, 7, 61, 0, 1, 0, 0, 5, 66, 0, 0, 1, 'Meryl salutes before leaving'
    UNION ALL SELECT 107976, 0, 7, 0, 61, 0, 1, 0, 0, 80, 10797601, 2, 0, 1, 'Play Meryl departure'
    UNION ALL SELECT 10797600, 9, 0, 0, 0, 0, 1, 6500, 6500, 22, 1, 0, 0, 1, 'Become ready to finish Preparations following the cast'
    UNION ALL SELECT 10797601, 9, 0, 0, 0, 0, 1, 1200, 1200, 11, 195453, 2, 0, 1, 'Meryl uses the original teleport visual'
    UNION ALL SELECT 10797601, 9, 1, 0, 0, 0, 1, 500, 500, 41, 0, 0, 0, 1, 'Remove introductory Meryl after the teleport visual'
) AS rows_to_add WHERE @original_intro;

INSERT INTO `conditions`
    (`SourceTypeOrReferenceId`, `SourceGroup`, `SourceEntry`, `SourceId`, `ElseGroup`,
     `ConditionTypeOrReference`, `ConditionTarget`, `ConditionValue1`, `ConditionValue2`, `ConditionValue3`,
     `NegativeCondition`, `ErrorTextId`, `ScriptName`, `Comment`)
SELECT conditions_to_add.* FROM (
    SELECT 22 AS `source_type`, 1 AS `source_group`, 107976 AS `source_entry`, 0 AS `source_id`,
        0 AS `else_group`, 44 AS `predicate`, 1 AS `target`, 1122 AS `value1`, 0 AS `value2`,
        0 AS `value3`, 0 AS `negate`, 0 AS `error_text`, '' AS `script`,
        'Start the introduction in the mage scenario Preparations stage' AS `description`
    UNION ALL SELECT 22, 1, 107976, 0, 0, 9, 0, 42479, 0, 0, 0, 0, '', 'Start the introduction for a player with The Mage Hunter'
    UNION ALL SELECT 22, 1, 107976, 0, 0, 36, 0, 0, 0, 0, 0, 0, '', 'The player listening to the introduction is alive'
    UNION ALL SELECT 22, 1, 107976, 0, 0, 32, 0, 16, 0, 0, 0, 0, '', 'Start the introduction for an actual player'
    UNION ALL SELECT 22, 6, 107976, 0, 0, 44, 1, 1122, 0, 0, 0, 0, '', 'Award the introduction only during the real Preparations stage'
    UNION ALL SELECT 22, 6, 107976, 0, 0, 9, 0, 42479, 0, 0, 0, 0, '', 'Preparations credit requires The Mage Hunter'
    UNION ALL SELECT 22, 6, 107976, 0, 0, 36, 0, 0, 0, 0, 0, 0, '', 'The player completing Preparations is alive'
    UNION ALL SELECT 22, 6, 107976, 0, 0, 32, 0, 16, 0, 0, 0, 0, '', 'Give Preparations credit to an actual player'
) AS conditions_to_add WHERE @original_intro;

COMMIT;
