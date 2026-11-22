-- The primer is read by its bearer during Learning the Language.
-- Its native self-cast and consumption are retained.
INSERT INTO `conditions`
    (`SourceTypeOrReferenceId`, `SourceGroup`, `SourceEntry`, `SourceId`, `ElseGroup`, `ConditionTypeOrReference`,
     `ConditionTarget`, `ConditionValue1`, `ConditionValue2`, `ConditionValue3`, `NegativeCondition`, `ErrorTextId`, `ScriptName`, `Comment`)
SELECT 17, 0, 30419, 0, 0, rules.`type`, 0, rules.`value1`, rules.`value2`, 0, 0, 0, '',
       'Stillpine primer: active Learning the Language and carried primer'
FROM (
    SELECT 9 AS `type`, 9538 AS `value1`, 0 AS `value2`
    UNION ALL SELECT 2, 23818, 1
) AS rules
WHERE NOT EXISTS (
    SELECT 1 FROM `conditions` existing
    WHERE existing.`SourceTypeOrReferenceId`=17 AND existing.`SourceGroup`=0 AND existing.`SourceEntry`=30419
      AND existing.`SourceId`=0 AND existing.`ElseGroup`=0 AND existing.`ConditionTypeOrReference`=rules.`type`
      AND existing.`ConditionTarget`=0 AND existing.`ConditionValue1`=rules.`value1` AND existing.`ConditionValue2`=rules.`value2`
      AND existing.`ConditionValue3`=0 AND existing.`NegativeCondition`=0
);
