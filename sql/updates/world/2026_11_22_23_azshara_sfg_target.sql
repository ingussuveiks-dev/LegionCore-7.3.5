-- Only the carried SFG of an accepted quest may target a living Enslaved Son.
INSERT INTO `conditions`
    (`SourceTypeOrReferenceId`,`SourceGroup`,`SourceEntry`,`SourceId`,`ElseGroup`,`ConditionTypeOrReference`,
     `ConditionTarget`,`ConditionValue1`,`ConditionValue2`,`ConditionValue3`,`NegativeCondition`,`ErrorTextId`,`ScriptName`,`Comment`)
SELECT 17,0,69743,0,0,rules.kind,rules.target,rules.value1,rules.value2,0,0,0,'',
       'In The Face: active quest, carried SFG and the actual living minefield target'
FROM (SELECT 9 AS kind,0 AS target,14472 AS value1,0 AS value2
      UNION ALL SELECT 2,0,49700,1
      UNION ALL SELECT 31,1,3,36868
      UNION ALL SELECT 36,1,0,0) AS rules
WHERE NOT EXISTS (
    SELECT 1 FROM `conditions` existing
    WHERE existing.`SourceTypeOrReferenceId`=17 AND existing.`SourceGroup`=0 AND existing.`SourceEntry`=69743
      AND existing.`SourceId`=0 AND existing.`ElseGroup`=0 AND existing.`ConditionTypeOrReference`=rules.kind
      AND existing.`ConditionTarget`=rules.target AND existing.`ConditionValue1`=rules.value1
      AND existing.`ConditionValue2`=rules.value2 AND existing.`ConditionValue3`=0 AND existing.`NegativeCondition`=0
);
