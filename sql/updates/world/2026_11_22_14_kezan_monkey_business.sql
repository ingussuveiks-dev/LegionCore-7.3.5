-- The banana cast requires its active quest and carried item, and may only
-- hit the living Bomb-Throwing Monkey used by the native sequence.
INSERT INTO `conditions`
    (`SourceTypeOrReferenceId`, `SourceGroup`, `SourceEntry`, `SourceId`, `ElseGroup`, `ConditionTypeOrReference`,
     `ConditionTarget`, `ConditionValue1`, `ConditionValue2`, `ConditionValue3`, `NegativeCondition`, `ErrorTextId`, `ScriptName`, `Comment`)
SELECT 17, 0, 67917, 0, 0, rules.`type`, rules.`target`, rules.`v1`, rules.`v2`, 0, 0, 0, '',
       'Monkey Business: living Bomb-Throwing Monkey and carried bananas during the quest'
FROM (
    SELECT 9 AS `type`, 0 AS `target`, 14019 AS `v1`, 0 AS `v2`
    UNION ALL SELECT 2, 0, 49028, 1
    UNION ALL SELECT 31, 1, 3, 34699
    UNION ALL SELECT 36, 1, 0, 0
) AS rules
WHERE NOT EXISTS (
    SELECT 1 FROM `conditions` existing
    WHERE existing.`SourceTypeOrReferenceId`=17 AND existing.`SourceGroup`=0 AND existing.`SourceEntry`=67917
      AND existing.`SourceId`=0 AND existing.`ElseGroup`=0 AND existing.`ConditionTypeOrReference`=rules.`type`
      AND existing.`ConditionTarget`=rules.`target` AND existing.`ConditionValue1`=rules.`v1`
      AND existing.`ConditionValue2`=rules.`v2` AND existing.`ConditionValue3`=0 AND existing.`NegativeCondition`=0
);

UPDATE `smart_scripts` SET `event_flags` = `event_flags` | 1
WHERE `entryorguid`=34699 AND `source_type`=0 AND `id`=0 AND `event_type`=8 AND `event_param1`=67917
  AND `action_type`=33 AND `action_param1`=35760;
