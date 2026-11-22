-- The actual cage and ravager already exist in the world. Gate the key cast.
INSERT INTO `conditions`
    (`SourceTypeOrReferenceId`,`SourceGroup`,`SourceEntry`,`SourceId`,`ElseGroup`,`ConditionTypeOrReference`,
     `ConditionTarget`,`ConditionValue1`,`ConditionValue2`,`ConditionValue3`,`NegativeCondition`,`ErrorTextId`,`ScriptName`,`Comment`)
SELECT 17,0,30767,0,0,rules.kind,rules.target,rules.value1,rules.value2,0,0,0,'',
       'Strength of One: accepted quest, carried ravager key and its cage'
FROM (SELECT 9 AS kind,0 AS target,9582 AS value1,0 AS value2
      UNION ALL SELECT 2,0,23925,1
      UNION ALL SELECT 31,1,5,181849) AS rules
WHERE NOT EXISTS (
    SELECT 1 FROM `conditions` existing
    WHERE existing.`SourceTypeOrReferenceId`=17 AND existing.`SourceGroup`=0 AND existing.`SourceEntry`=30767
      AND existing.`SourceId`=0 AND existing.`ElseGroup`=0 AND existing.`ConditionTypeOrReference`=rules.kind
      AND existing.`ConditionTarget`=rules.target AND existing.`ConditionValue1`=rules.value1
      AND existing.`ConditionValue2`=rules.value2 AND existing.`ConditionValue3`=0 AND existing.`NegativeCondition`=0
);
