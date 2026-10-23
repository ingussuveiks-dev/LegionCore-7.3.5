START TRANSACTION;

SET @marksman_native_pickup := EXISTS (
    SELECT 1 FROM gameobject_template AS object
    JOIN gameobject_loot_template AS loot ON loot.Entry=object.Data1 AND loot.Item=128826
    JOIN smart_scripts AS pickup ON pickup.entryorguid=object.entry AND pickup.source_type=1 AND pickup.id=0
    JOIN smart_scripts AS criterion ON criterion.entryorguid=object.entry AND criterion.source_type=1 AND criterion.id=1
    JOIN smart_scripts AS replay ON replay.entryorguid=object.entry AND replay.source_type=1 AND replay.id=2
    JOIN smart_scripts AS claim ON claim.entryorguid=object.entry AND claim.source_type=1 AND claim.id=3
    WHERE object.entry=248419 AND object.type=3 AND object.Data1=248419
      AND object.AIName='SmartGameObjectAI' AND object.ScriptName IN('','go_artifact_acquisition')
      AND loot.Currency=0 AND loot.Reference=0 AND loot.Chance=100 AND loot.MinCount=1 AND loot.MaxCount=1
      AND pickup.event_type=70 AND pickup.event_param1=3 AND pickup.link=1
      AND pickup.action_type=85 AND pickup.action_param1=200279 AND pickup.target_type=7
      AND criterion.event_type=61 AND criterion.link=3 AND criterion.action_type=205
      AND criterion.action_param1=92 AND criterion.action_param2=48210 AND criterion.target_type=7
      AND replay.event_type=64 AND replay.event_param1=3 AND replay.link=1
      AND replay.action_type=85 AND replay.action_param1=200279 AND replay.target_type=7
      AND claim.event_type=61 AND claim.link=0 AND claim.action_type=33
      AND claim.action_param1=103490 AND claim.target_type=7
);

UPDATE gameobject_template SET ScriptName='go_artifact_acquisition'
WHERE entry=248419 AND ScriptName='' AND @marksman_native_pickup;

INSERT INTO conditions (SourceTypeOrReferenceId,SourceGroup,SourceEntry,SourceId,ElseGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2,ConditionValue3,NegativeCondition,ErrorTextId,ScriptName,Comment)
SELECT scope.source_type,scope.source_group,scope.source_entry,scope.source_id,variant.else_group,
       IF(rule.condition_type=0,variant.quest_type,rule.condition_type),0,rule.value1,
       CASE rule.condition_type WHEN 44 THEN variant.stage WHEN 2 THEN 1 ELSE 0 END,
       IF(rule.condition_type=2,1,0),rule.negative,0,'','Marksmanship: active Hunter final-stage native loot and claimed-item event'
FROM (
    SELECT 4 AS source_type,248419 AS source_group,128826 AS source_entry,0 AS source_id
    UNION ALL SELECT 22,1,248419,1
) AS scope
CROSS JOIN (
    SELECT 0 AS else_group,7 AS stage,9 AS quest_type
    UNION ALL SELECT 1,7,28
    UNION ALL SELECT 2,8,9
    UNION ALL SELECT 3,8,28
) AS variant
CROSS JOIN (
    SELECT 2 AS condition_type,128826 AS value1,0 AS negative
    UNION ALL SELECT 15,4,0
    UNION ALL SELECT 22,1489,0
    UNION ALL SELECT 44,972,0
    UNION ALL SELECT 0,40419,0
    UNION ALL SELECT 8,40419,1
) AS rule
WHERE @marksman_native_pickup AND (scope.source_type=22 OR rule.condition_type<>2)
  AND NOT EXISTS (
      SELECT 1 FROM conditions
      WHERE SourceTypeOrReferenceId=scope.source_type AND SourceGroup=scope.source_group
        AND SourceEntry=scope.source_entry AND SourceId=scope.source_id
  );

COMMIT;
