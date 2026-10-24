START TRANSACTION;

SET @xalatath_native_roots := EXISTS (
    SELECT 1 FROM gameobject_template WHERE entry=247690 AND type=3 AND Data0=1691 AND Data1=247690
      AND AIName='SmartGameObjectAI' AND ScriptName=''
) AND (SELECT COUNT(*) FROM smart_scripts WHERE entryorguid=247690 AND source_type=1)=5
AND (SELECT COUNT(*) FROM smart_scripts WHERE entryorguid=247690 AND source_type=1
    AND event_phase_mask=0 AND event_chance=100 AND target_type=7
    AND ((id=0 AND link=1 AND event_type=70 AND event_param1=3 AND event_flags=0 AND action_type=85 AND action_param1=201780 AND action_param2=0)
      OR (id=1 AND link=3 AND event_type=61 AND event_param1=0 AND event_flags=0 AND action_type=205 AND action_param1=92 AND action_param2=48493)
      OR (id=2 AND link=1 AND event_type=64 AND event_param1=3 AND event_flags=1 AND action_type=85 AND action_param1=201780 AND action_param2=0)
      OR (id=3 AND link=4 AND event_type=61 AND event_flags=0 AND action_type=85 AND action_param1=204919 AND action_param2=0)
      OR (id=4 AND link=0 AND event_type=61 AND event_flags=0 AND action_type=85 AND action_param1=201917 AND action_param2=0)))=5
AND (SELECT COUNT(*) FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceEntry=247690 AND SourceId=1)=1
AND EXISTS (
    SELECT 1 FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceGroup=3 AND SourceEntry=247690 AND SourceId=1
      AND ElseGroup=0 AND ConditionTypeOrReference=2 AND ConditionTarget=0 AND ConditionValue1=128827 AND ConditionValue2=1
      AND ConditionValue3=0 AND NegativeCondition=0 AND ErrorTextId=0 AND ScriptName=''
);

DELETE FROM conditions WHERE @xalatath_native_roots
  AND SourceTypeOrReferenceId=22 AND SourceGroup=3 AND SourceEntry=247690 AND SourceId=1;

INSERT INTO conditions (SourceTypeOrReferenceId,SourceGroup,SourceEntry,SourceId,ElseGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2,ConditionValue3,NegativeCondition,ErrorTextId,ScriptName,Comment)
SELECT 22,root.source_group,247690,1,variant.else_group,
       IF(rule.condition_type=0,variant.quest_type,rule.condition_type),0,rule.value1,
       CASE rule.condition_type WHEN 44 THEN variant.stage WHEN 2 THEN 1 ELSE 0 END,
       0,rule.negative,0,'','Xalatath: living active Priest final-stage owned progression'
FROM (SELECT 1 AS source_group UNION ALL SELECT 3) AS root
CROSS JOIN (
    SELECT 0 AS else_group,8 AS stage,9 AS quest_type
    UNION ALL SELECT 1,8,28
    UNION ALL SELECT 2,9,9
    UNION ALL SELECT 3,9,28
) AS variant
CROSS JOIN (
    SELECT 2 AS condition_type,128827 AS value1,0 AS negative
    UNION ALL SELECT 15,16,0
    UNION ALL SELECT 22,1539,0
    UNION ALL SELECT 44,991,0
    UNION ALL SELECT 0,40710,0
    UNION ALL SELECT 8,40710,1
    UNION ALL SELECT 36,0,0
) AS rule
WHERE @xalatath_native_roots;

COMMIT;
