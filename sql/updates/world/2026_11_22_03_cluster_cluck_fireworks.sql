-- The item cast uses the same target as the original spell-click. Preserve the click condition.
INSERT INTO `conditions`
    (`SourceTypeOrReferenceId`, `SourceGroup`, `SourceEntry`, `SourceId`, `ElseGroup`,
     `ConditionTypeOrReference`, `ConditionTarget`, `ConditionValue1`, `ConditionValue2`, `ConditionValue3`,
     `NegativeCondition`, `ErrorTextId`, `ScriptName`, `Comment`)
SELECT 17, 0, 71170, 0, 0, rules.`type`, rules.`target`, rules.`value1`, rules.`value2`, 0, 0, 0, '', rules.`comment`
FROM (
    SELECT 9 AS `type`, 0 AS `target`, 24671 AS `value1`, 0 AS `value2`, 'Remote Control Fireworks - active Cluster Cluck' AS `comment`
    UNION ALL SELECT 2, 0, 52712, 1, 'Remote Control Fireworks - carried quest item'
    UNION ALL SELECT 31, 1, 3, 38111, 'Remote Control Fireworks - Wild Clucker target'
    UNION ALL SELECT 36, 1, 0, 0, 'Remote Control Fireworks - living target'
) AS rules
WHERE NOT EXISTS (
    SELECT 1 FROM `conditions` AS existing
    WHERE existing.`SourceTypeOrReferenceId` = 17 AND existing.`SourceGroup` = 0
      AND existing.`SourceEntry` = 71170 AND existing.`SourceId` = 0 AND existing.`ElseGroup` = 0
      AND existing.`ConditionTypeOrReference` = rules.`type` AND existing.`ConditionTarget` = rules.`target`
      AND existing.`ConditionValue1` = rules.`value1` AND existing.`ConditionValue2` = rules.`value2`
      AND existing.`ConditionValue3` = 0
);
