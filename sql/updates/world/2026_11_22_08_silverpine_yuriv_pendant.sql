-- The supplied pendant may satisfy the inventory objective on acceptance.
-- Both active quest states can deliver it to the one original receiver.
INSERT INTO `conditions`
    (`SourceTypeOrReferenceId`, `SourceGroup`, `SourceEntry`, `SourceId`, `ElseGroup`, `ConditionTypeOrReference`,
     `ConditionTarget`, `ConditionValue1`, `ConditionValue2`, `ConditionValue3`, `NegativeCondition`, `ErrorTextId`, `ScriptName`, `Comment`)
SELECT 17, 0, 8001, 0, rules.`branch`, rules.`type`, rules.`target`, rules.`value1`, rules.`value2`, 0, 0, 0, '',
       'Clarice''s Pendant - accepted quest, carried pendant and Yuriv''s Tombstone'
FROM (
    SELECT 0 AS `branch`, 9 AS `type`, 0 AS `target`, 264 AS `value1`, 0 AS `value2`
    UNION ALL SELECT 0, 2, 0, 6145, 1
    UNION ALL SELECT 0, 31, 1, 5, 24776
    UNION ALL SELECT 1, 28, 0, 264, 0
    UNION ALL SELECT 1, 2, 0, 6145, 1
    UNION ALL SELECT 1, 31, 1, 5, 24776
) AS rules
WHERE NOT EXISTS (
    SELECT 1 FROM `conditions` existing
    WHERE existing.`SourceTypeOrReferenceId` = 17 AND existing.`SourceGroup` = 0 AND existing.`SourceEntry` = 8001
      AND existing.`SourceId` = 0 AND existing.`ElseGroup` = rules.`branch`
      AND existing.`ConditionTypeOrReference` = rules.`type` AND existing.`ConditionTarget` = rules.`target`
      AND existing.`ConditionValue1` = rules.`value1` AND existing.`ConditionValue2` = rules.`value2`
      AND existing.`ConditionValue3` = 0 AND existing.`NegativeCondition` = 0
);
