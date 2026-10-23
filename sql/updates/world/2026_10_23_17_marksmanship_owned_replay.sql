START TRANSACTION;

SET @marksman_owned_replay := (
    SELECT COUNT(*)=1 AND SUM(ElseGroup=0 AND ConditionTypeOrReference=2 AND ConditionTarget=0
        AND ConditionValue1=128826 AND ConditionValue2=1 AND ConditionValue3=0 AND NegativeCondition=0
        AND ErrorTextId=0 AND ScriptName='')=1
    FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceGroup=3 AND SourceEntry=248419 AND SourceId=1
) AND EXISTS (
    SELECT 1 FROM gameobject_template AS object
    JOIN smart_scripts AS replay ON replay.entryorguid=object.entry AND replay.source_type=1 AND replay.id=2
    WHERE object.entry=248419 AND object.type=3 AND object.Data1=248419
      AND object.AIName='SmartGameObjectAI' AND object.ScriptName=''
      AND replay.event_type=64 AND replay.link=1 AND replay.action_type=85
      AND replay.action_param1=200279 AND replay.target_type=7
);

DELETE FROM conditions
WHERE SourceTypeOrReferenceId=22 AND SourceGroup=3 AND SourceEntry=248419 AND SourceId=1 AND @marksman_owned_replay;

INSERT INTO conditions (SourceTypeOrReferenceId,SourceGroup,SourceEntry,SourceId,ElseGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2,ConditionValue3,NegativeCondition,ErrorTextId,ScriptName,Comment)
SELECT 22,3,248419,1,variant.else_group,IF(rule.condition_type=0,variant.quest_type,rule.condition_type),0,rule.value1,
       CASE rule.condition_type WHEN 44 THEN variant.stage WHEN 2 THEN 1 ELSE 0 END,
       IF(rule.condition_type=2,1,0),rule.negative,0,'','Marksmanship: final-stage owned or bank artifact replay'
FROM (
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
WHERE @marksman_owned_replay;

COMMIT;
