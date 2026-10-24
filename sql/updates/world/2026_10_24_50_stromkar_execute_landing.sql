START TRANSACTION;

SET @stromkar_execute_landing := EXISTS (SELECT 1 FROM creature_template WHERE entry=104276
    AND AIName='SmartAI' AND ScriptName='npc_stromkar_zakajz')
AND EXISTS (SELECT 1 FROM gameobject_template WHERE entry=247877 AND type=3 AND Data1=247877
    AND AIName='SmartGameObjectAI' AND ScriptName='go_stromkar_pickup')
AND (SELECT COUNT(*) FROM conditions WHERE SourceTypeOrReferenceId=13 AND SourceEntry=206455)=22
AND (SELECT COUNT(*) FROM conditions current
    JOIN (SELECT 0 AS else_group,9 AS quest_type UNION ALL SELECT 1,28) AS variant ON current.ElseGroup=variant.else_group
    JOIN (SELECT 31 AS condition_type,0 AS target,3 AS value1,104276 AS value2,0 AS negative
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
        ON current.ConditionTypeOrReference=IF(rule.condition_type=0,variant.quest_type,rule.condition_type)
        AND current.ConditionTarget=rule.target AND current.ConditionValue1=rule.value1 AND current.ConditionValue2=rule.value2
        AND current.NegativeCondition=rule.negative
    WHERE current.SourceTypeOrReferenceId=13 AND current.SourceEntry=206455 AND current.SourceGroup=1
        AND current.SourceId=0 AND current.ConditionValue3=0 AND current.ErrorTextId=0 AND current.ScriptName='')=22
AND NOT EXISTS (SELECT 1 FROM conditions WHERE SourceTypeOrReferenceId IN(13,17) AND SourceEntry=206456)
AND NOT EXISTS (SELECT 1 FROM spell_target_position WHERE id=206456)
AND NOT EXISTS (SELECT 1 FROM spell_script_names WHERE spell_id=206456);

INSERT INTO conditions (SourceTypeOrReferenceId,SourceGroup,SourceEntry,SourceId,ElseGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2,ConditionValue3,NegativeCondition,ErrorTextId,ScriptName,Comment)
SELECT 17,0,206456,0,variant.else_group,IF(rule.condition_type=0,variant.quest_type,rule.condition_type),
    0,rule.value1,rule.value2,0,rule.negative,0,'',
    'Stromkar Execute: revalidate active living equipped Warrior at native jump arrival'
FROM (SELECT 0 AS else_group,9 AS quest_type UNION ALL SELECT 1,28) AS variant
CROSS JOIN (SELECT 15 AS condition_type,1 AS value1,0 AS value2,0 AS negative
    UNION ALL SELECT 22,1539,0,0
    UNION ALL SELECT 44,1037,9,0
    UNION ALL SELECT 0,41105,0,0
    UNION ALL SELECT 8,41105,0,1
    UNION ALL SELECT 3,128910,0,0
    UNION ALL SELECT 36,0,0,0
    UNION ALL SELECT 1,206448,0,0) AS rule
WHERE @stromkar_execute_landing;

COMMIT;
