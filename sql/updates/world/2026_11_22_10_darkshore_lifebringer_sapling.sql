-- The sapling's two explicit effects may target only the four quest animals.
INSERT INTO `conditions`
    (`SourceTypeOrReferenceId`, `SourceGroup`, `SourceEntry`, `SourceId`, `ElseGroup`, `ConditionTypeOrReference`,
     `ConditionTarget`, `ConditionValue1`, `ConditionValue2`, `ConditionValue3`, `NegativeCondition`, `ErrorTextId`, `ScriptName`, `Comment`)
SELECT 17, 0, 65188, 0, animals.`group_id`, rules.`type`, rules.`target`,
       IF(rules.`type` = 31, 3, rules.`v1`), IF(rules.`type` = 31, animals.`entry`, rules.`v2`), 0, 0, 0, '',
       'Lifebringer Sapling - accepted quest, carried sapling and living native animal'
FROM (
    SELECT 0 AS `group_id`, 2165 AS `entry`
    UNION ALL SELECT 1, 2237
    UNION ALL SELECT 2, 34318
    UNION ALL SELECT 3, 2071
) AS animals
CROSS JOIN (
    SELECT 9 AS `type`, 0 AS `target`, 13925 AS `v1`, 0 AS `v2`
    UNION ALL SELECT 2, 0, 46363, 1
    UNION ALL SELECT 31, 1, 0, 0
    UNION ALL SELECT 36, 1, 0, 0
) AS rules
WHERE NOT EXISTS (
    SELECT 1 FROM `conditions` existing
    WHERE existing.`SourceTypeOrReferenceId` = 17 AND existing.`SourceGroup` = 0
      AND existing.`SourceEntry` = 65188 AND existing.`SourceId` = 0 AND existing.`ElseGroup` = animals.`group_id`
      AND existing.`ConditionTypeOrReference` = rules.`type` AND existing.`ConditionTarget` = rules.`target`
      AND existing.`ConditionValue1` = IF(rules.`type` = 31, 3, rules.`v1`)
      AND existing.`ConditionValue2` = IF(rules.`type` = 31, animals.`entry`, rules.`v2`)
      AND existing.`ConditionValue3` = 0 AND existing.`NegativeCondition` = 0
);

-- Keep the matriarch's existing cub and Sprint events. Copy the corresponding
-- spell-hit sequence from a supported animal into unused event slots.
INSERT INTO `smart_scripts`
    (`entryorguid`, `source_type`, `id`, `link`, `Difficulties`, `event_type`, `event_phase_mask`, `event_chance`,
     `event_flags`, `event_param1`, `event_param2`, `event_param3`, `event_param4`, `event_param5`,
     `action_type`, `action_param1`, `action_param2`, `action_param3`, `action_param4`, `action_param5`, `action_param6`,
     `target_type`, `target_param1`, `target_param2`, `target_param3`, `target_param4`, `target_x`, `target_y`, `target_z`, `target_o`, `comment`)
SELECT 2071, original.`source_type`, original.`id` + 2, IF(original.`link` = 0, 0, original.`link` + 2),
       original.`Difficulties`, original.`event_type`, original.`event_phase_mask`, original.`event_chance`, original.`event_flags`,
       original.`event_param1`, original.`event_param2`, original.`event_param3`, original.`event_param4`, original.`event_param5`,
       original.`action_type`, original.`action_param1`, original.`action_param2`, original.`action_param3`, original.`action_param4`,
       original.`action_param5`, original.`action_param6`, original.`target_type`, original.`target_param1`, original.`target_param2`,
       original.`target_param3`, original.`target_param4`, original.`target_x`, original.`target_y`, original.`target_z`, original.`target_o`,
       CONCAT('Moonstalker Matriarch - Lifebringer Sapling - ', original.`comment`)
FROM `smart_scripts` original
WHERE original.`entryorguid` = 2165 AND original.`source_type` = 0 AND original.`id` BETWEEN 0 AND 3
  AND NOT EXISTS (
      SELECT 1 FROM `smart_scripts` existing
      WHERE existing.`entryorguid` = 2071 AND existing.`source_type` = 0 AND existing.`id` = original.`id` + 2
  );
