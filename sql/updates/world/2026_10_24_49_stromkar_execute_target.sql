START TRANSACTION;

SET @stromkar_execute_target := EXISTS (SELECT 1 FROM creature_template WHERE entry=104276
    AND AIName='SmartAI' AND ScriptName='npc_stromkar_zakajz')
AND EXISTS (SELECT 1 FROM gameobject_template WHERE entry=247877 AND type=3 AND Data1=247877
    AND AIName='SmartGameObjectAI' AND ScriptName='go_stromkar_pickup')
AND NOT EXISTS (SELECT 1 FROM conditions WHERE SourceTypeOrReferenceId=13 AND SourceEntry=206455)
AND NOT EXISTS (SELECT 1 FROM spell_target_position WHERE id=206455)
AND NOT EXISTS (SELECT 1 FROM spell_script_names WHERE spell_id=206455);

INSERT INTO conditions (SourceTypeOrReferenceId,SourceGroup,SourceEntry,SourceId,ElseGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2,ConditionValue3,NegativeCondition,ErrorTextId,ScriptName,Comment)
SELECT 13,1,206455,0,variant.else_group,IF(rule.condition_type=0,variant.quest_type,rule.condition_type),
    rule.target,rule.value1,rule.value2,0,rule.negative,0,'',
    'Stromkar Execute: native nearby destination is living defeated Zakajz in final stage'
FROM (SELECT 0 AS else_group,9 AS quest_type UNION ALL SELECT 1,28) AS variant
CROSS JOIN (SELECT 31 AS condition_type,0 AS target,3 AS value1,104276 AS value2,0 AS negative
    UNION ALL SELECT 22,0,1539,0,0
    UNION ALL SELECT 44,0,1037,9,0
    UNION ALL SELECT 36,0,0,0,0
    UNION ALL SELECT 38,0,1,4,0
    UNION ALL SELECT 15,1,1,0,0
    UNION ALL SELECT 0,1,41105,0,0
    UNION ALL SELECT 8,1,41105,0,1
    UNION ALL SELECT 3,1,128910,0,0
    UNION ALL SELECT 36,1,0,0,0
    UNION ALL SELECT 1,1,206448,0,0) AS rule
WHERE @stromkar_execute_target;

COMMIT;
