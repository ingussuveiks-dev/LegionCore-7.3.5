-- The current quest and an already accepted legacy counterpart share the native lock.
INSERT INTO `conditions`
    (`SourceTypeOrReferenceId`, `SourceGroup`, `SourceEntry`, `SourceId`, `ElseGroup`, `ConditionTypeOrReference`,
     `ConditionTarget`, `ConditionValue1`, `ConditionValue2`, `ConditionValue3`, `NegativeCondition`, `ErrorTextId`, `ScriptName`, `Comment`)
SELECT 17, 0, 29773, 0, variants.`groupId`, rules.`type`, rules.`target`,
       IF(rules.`type`=9, variants.`quest`, rules.`v1`), rules.`v2`, 0, 0, 0, '',
       'Reclaiming Felfire Hill: carried seedlings and the Fertile Dirt Mound'
FROM (
    SELECT 0 AS `groupId`, 26446 AS `quest`
    UNION ALL SELECT 1, 9526
) AS variants
CROSS JOIN (
    SELECT 9 AS `type`, 0 AS `target`, 0 AS `v1`, 0 AS `v2`
    UNION ALL SELECT 2, 0, 23788, 1
    UNION ALL SELECT 31, 1, 5, 181690
) AS rules
WHERE NOT EXISTS (
    SELECT 1 FROM `conditions` existing
    WHERE existing.`SourceTypeOrReferenceId`=17 AND existing.`SourceGroup`=0 AND existing.`SourceEntry`=29773
      AND existing.`SourceId`=0 AND existing.`ElseGroup`=variants.`groupId` AND existing.`ConditionTypeOrReference`=rules.`type`
      AND existing.`ConditionTarget`=rules.`target`
      AND existing.`ConditionValue1`=IF(rules.`type`=9, variants.`quest`, rules.`v1`)
      AND existing.`ConditionValue2`=rules.`v2` AND existing.`ConditionValue3`=0 AND existing.`NegativeCondition`=0
);
