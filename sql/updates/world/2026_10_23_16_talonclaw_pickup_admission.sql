INSERT INTO conditions (SourceTypeOrReferenceId,SourceGroup,SourceEntry,SourceId,ElseGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2,ConditionValue3,NegativeCondition,ErrorTextId,ScriptName,Comment)
SELECT 4,248834,128808,0,variant.else_group,
       IF(rule.condition_type=0,variant.quest_type,rule.condition_type),0,rule.value1,
       IF(rule.condition_type=44,variant.stage,0),0,rule.negative,0,'','Talonclaw: active Hunter final-stage loot admission'
FROM gameobject_template AS object
JOIN gameobject_loot_template AS loot ON loot.Entry=object.Data1 AND loot.Item=128808
CROSS JOIN (
    SELECT 0 AS else_group,12 AS stage,9 AS quest_type
    UNION ALL SELECT 1,12,28
    UNION ALL SELECT 2,13,9
    UNION ALL SELECT 3,13,28
) AS variant
CROSS JOIN (
    SELECT 15 AS condition_type,4 AS value1,0 AS negative
    UNION ALL SELECT 22,1511,0
    UNION ALL SELECT 44,973,0
    UNION ALL SELECT 0,40385,0
    UNION ALL SELECT 8,40385,1
) AS rule
WHERE object.entry=248834 AND object.type=3 AND object.Data1=248834
  AND object.ScriptName='go_artifact_acquisition' AND object.AIName='SmartGameObjectAI'
  AND loot.Currency=0 AND loot.Reference=0 AND loot.MinCount=1 AND loot.MaxCount=1
  AND NOT EXISTS (SELECT 1 FROM conditions WHERE SourceTypeOrReferenceId=4 AND SourceGroup=248834 AND SourceEntry=128808 AND SourceId=0);
