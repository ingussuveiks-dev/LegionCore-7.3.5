-- All three native selectors on Throw Torch must agree on the live trough.
INSERT INTO `conditions`
    (`SourceTypeOrReferenceId`, `SourceGroup`, `SourceEntry`, `SourceId`, `ElseGroup`, `ConditionTypeOrReference`,
     `ConditionTarget`, `ConditionValue1`, `ConditionValue2`, `ConditionValue3`, `NegativeCondition`, `ErrorTextId`, `ScriptName`, `Comment`)
SELECT rules.`sourceType`,rules.`sourceGroup`,69228,0,rules.`elseGroup`,rules.`type`,0,rules.`v1`,rules.`v2`,0,0,0,'',
       'Feed of Evil: the active quest, carried torch and true living trough targets'
FROM (
    SELECT 17 AS `sourceType`,0 AS `sourceGroup`,0 AS `elseGroup`,9 AS `type`,14461 AS `v1`,0 AS `v2`
    UNION ALL SELECT 17,0,0,2,49539,1
    UNION ALL SELECT 13,7,0,31,3,36727
    UNION ALL SELECT 13,7,0,36,0,0
    UNION ALL SELECT 13,7,1,31,3,37155
    UNION ALL SELECT 13,7,1,36,0,0
    UNION ALL SELECT 13,7,2,31,3,37156
    UNION ALL SELECT 13,7,2,36,0,0
) AS rules
WHERE NOT EXISTS (
    SELECT 1 FROM `conditions` existing
    WHERE existing.`SourceTypeOrReferenceId`=rules.`sourceType` AND existing.`SourceGroup`=rules.`sourceGroup`
      AND existing.`SourceEntry`=69228 AND existing.`SourceId`=0 AND existing.`ElseGroup`=rules.`elseGroup`
      AND existing.`ConditionTypeOrReference`=rules.`type` AND existing.`ConditionTarget`=0
      AND existing.`ConditionValue1`=rules.`v1` AND existing.`ConditionValue2`=rules.`v2`
      AND existing.`ConditionValue3`=0 AND existing.`NegativeCondition`=0
);
