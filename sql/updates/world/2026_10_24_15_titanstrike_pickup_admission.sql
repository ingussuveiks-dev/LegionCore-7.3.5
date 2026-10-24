START TRANSACTION;

SET @titanstrike_native_pickup := EXISTS (
    SELECT 1 FROM gameobject_template AS object
    JOIN gameobject_loot_template AS loot ON loot.Entry=object.Data1 AND loot.Item=128861
    WHERE object.entry=249812 AND object.type=3 AND object.Data1=249812
      AND object.AIName='SmartGameObjectAI' AND object.ScriptName='go_artifact_acquisition'
      AND loot.Currency=0 AND loot.Reference=0 AND loot.Chance=100
      AND loot.MinCount=1 AND loot.MaxCount=1
) AND (SELECT COUNT(*) FROM smart_scripts WHERE entryorguid=249812 AND source_type=1)=4
AND (SELECT COUNT(*) FROM smart_scripts
     WHERE entryorguid=249812 AND source_type=1 AND event_chance=100
       AND event_flags=1 AND event_phase_mask=0
       AND action_param3=0 AND action_param4=0 AND action_param5=0 AND action_param6=0
       AND target_param1=0 AND target_param2=0 AND target_param3=0
       AND (
           (id=0 AND link=1 AND event_type=70 AND event_param1=3
            AND action_type=85 AND action_param1=212010 AND action_param2=0 AND target_type=7)
        OR (id=1 AND link=3 AND event_type=61 AND action_type=205
            AND action_param1=92 AND action_param2=50923 AND target_type=7)
        OR (id=3 AND link=4 AND event_type=61 AND action_type=12
            AND action_param1=106854 AND action_param2=8 AND target_type=8)
        OR (id=4 AND link=0 AND event_type=61 AND action_type=28
            AND action_param1=211970 AND action_param2=0 AND target_type=7)
       ))=4;

INSERT INTO conditions (SourceTypeOrReferenceId,SourceGroup,SourceEntry,SourceId,ElseGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2,ConditionValue3,NegativeCondition,ErrorTextId,ScriptName,Comment)
SELECT scope.source_type,scope.source_group,scope.source_entry,scope.source_id,variant.else_group,
       IF(rule.condition_type=0,variant.quest_type,rule.condition_type),0,rule.value1,
       CASE rule.condition_type WHEN 44 THEN variant.stage WHEN 2 THEN 1 ELSE 0 END,
       IF(rule.condition_type=2,1,0),rule.negative,0,'','Titanstrike: active Hunter final-stage loot and owned completed-loot event'
FROM (
    SELECT 4 AS source_type,249812 AS source_group,128861 AS source_entry,0 AS source_id
    UNION ALL SELECT 22,1,249812,1
) AS scope
CROSS JOIN (
    SELECT 0 AS else_group,4 AS stage,9 AS quest_type
    UNION ALL SELECT 1,4,28
    UNION ALL SELECT 2,5,9
    UNION ALL SELECT 3,5,28
) AS variant
CROSS JOIN (
    SELECT 2 AS condition_type,128861 AS value1,0 AS negative
    UNION ALL SELECT 15,4,0
    UNION ALL SELECT 22,1609,0
    UNION ALL SELECT 44,1099,0
    UNION ALL SELECT 0,42185,0
    UNION ALL SELECT 8,42185,1
) AS rule
WHERE @titanstrike_native_pickup AND (scope.source_type=22 OR rule.condition_type<>2)
  AND NOT EXISTS (
      SELECT 1 FROM conditions
      WHERE SourceTypeOrReferenceId=scope.source_type AND SourceGroup=scope.source_group
        AND SourceEntry=scope.source_entry AND SourceId=scope.source_id
  );

COMMIT;
