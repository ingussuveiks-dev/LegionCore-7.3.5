-- Keep the native effect and the existing Razormane SmartAI graph.
-- The Grol'dom Net can only hit a living pillager while its quest is active.
INSERT INTO `conditions`
    (`SourceTypeOrReferenceId`, `SourceGroup`, `SourceEntry`, `SourceId`, `ElseGroup`, `ConditionTypeOrReference`,
     `ConditionTarget`, `ConditionValue1`, `ConditionValue2`, `ConditionValue3`, `NegativeCondition`, `ErrorTextId`, `ScriptName`, `Comment`)
SELECT 17, 0, 65580, 0, 0, rules.`type`, rules.`target`, rules.`v1`, rules.`v2`, 0, 0, 0, '',
       'Drag it Out of Them: carried Grol''dom Net, active quest and living Razormane Pillager'
FROM (
    SELECT 9 AS `type`, 0 AS `target`, 13961 AS `v1`, 0 AS `v2`
    UNION ALL SELECT 2, 0, 46722, 1
    UNION ALL SELECT 31, 1, 3, 34503
    UNION ALL SELECT 36, 1, 0, 0
) AS rules
WHERE NOT EXISTS (
    SELECT 1 FROM `conditions` existing
    WHERE existing.`SourceTypeOrReferenceId`=17 AND existing.`SourceGroup`=0 AND existing.`SourceEntry`=65580
      AND existing.`SourceId`=0 AND existing.`ElseGroup`=0 AND existing.`ConditionTypeOrReference`=rules.`type`
      AND existing.`ConditionTarget`=rules.`target` AND existing.`ConditionValue1`=rules.`v1`
      AND existing.`ConditionValue2`=rules.`v2` AND existing.`ConditionValue3`=0 AND existing.`NegativeCondition`=0
);
