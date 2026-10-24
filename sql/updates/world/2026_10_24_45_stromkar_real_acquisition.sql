START TRANSACTION;

SET @stromkar_missing_loot := EXISTS (SELECT 1 FROM gameobject_template
    WHERE entry=247877 AND type=3 AND Data0=1691 AND Data1=0 AND Data14=23645
      AND AIName='SmartGameObjectAI' AND ScriptName='')
AND (SELECT COUNT(*) FROM gameobject_loot_template WHERE Entry=247877)=1
AND EXISTS (SELECT 1 FROM gameobject_loot_template WHERE Entry=247877 AND Item=128910
    AND Currency=0 AND Reference=0 AND Chance=100 AND QuestRequired=0 AND LootMode=1 AND GroupId=0 AND MinCount=1 AND MaxCount=1)
AND (SELECT COUNT(*) FROM smart_scripts WHERE entryorguid=247877 AND source_type=1)=3
AND (SELECT COUNT(*) FROM smart_scripts WHERE entryorguid=247877 AND source_type=1
    AND Difficulties='' AND event_phase_mask=0 AND event_chance=100 AND event_flags=0 AND target_type=7
    AND ((id=0 AND link=1 AND event_type=70 AND event_param1=3 AND action_type=205 AND action_param1=92 AND action_param2=49110)
      OR (id=1 AND link=2 AND event_type=61 AND action_type=85 AND action_param1=206436 AND action_param2=0)
      OR (id=2 AND link=0 AND event_type=61 AND action_type=85 AND action_param1=206448 AND action_param2=0)))=3
AND NOT EXISTS (SELECT 1 FROM conditions WHERE (SourceTypeOrReferenceId=4 AND SourceGroup=247877)
    OR (SourceTypeOrReferenceId=22 AND SourceEntry=247877 AND SourceId=1)
    OR (SourceTypeOrReferenceId=17 AND SourceEntry IN(206436,206448)))
AND NOT EXISTS (SELECT 1 FROM spell_script_names WHERE spell_id IN(206436,206448));

INSERT INTO conditions (SourceTypeOrReferenceId,SourceGroup,SourceEntry,SourceId,ElseGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2,ConditionValue3,NegativeCondition,ErrorTextId,ScriptName,Comment)
SELECT 4,247877,128910,0,variant.else_group,IF(rule.condition_type=0,variant.quest_type,rule.condition_type),0,
    rule.value1,IF(rule.condition_type=44,9,0),0,rule.negative,0,'','Stromkar: active Warrior final-stage native real loot'
FROM (SELECT 0 AS else_group,9 AS quest_type UNION ALL SELECT 1,28) AS variant
CROSS JOIN (SELECT 15 AS condition_type,1 AS value1,0 AS negative
    UNION ALL SELECT 22,1539,0
    UNION ALL SELECT 44,1037,0
    UNION ALL SELECT 0,41105,0
    UNION ALL SELECT 8,41105,1) AS rule
WHERE @stromkar_missing_loot;

INSERT INTO conditions (SourceTypeOrReferenceId,SourceGroup,SourceEntry,SourceId,ElseGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2,ConditionValue3,NegativeCondition,ErrorTextId,ScriptName,Comment)
SELECT consumer.source_type,consumer.source_group,consumer.source_entry,consumer.source_id,variant.else_group,
    IF(rule.condition_type=0,variant.quest_type,rule.condition_type),0,rule.value1,
    CASE rule.condition_type WHEN 44 THEN 9 WHEN 2 THEN 1 ELSE 0 END,0,rule.negative,0,'',
    'Stromkar: living active Warrior final-stage owned progression and delayed cast'
FROM (SELECT 22 AS source_type,1 AS source_group,247877 AS source_entry,1 AS source_id
    UNION ALL SELECT 17,0,206436,0
    UNION ALL SELECT 17,0,206448,0) AS consumer
CROSS JOIN (SELECT 0 AS else_group,9 AS quest_type UNION ALL SELECT 1,28) AS variant
CROSS JOIN (SELECT 2 AS condition_type,128910 AS value1,0 AS negative
    UNION ALL SELECT 15,1,0
    UNION ALL SELECT 22,1539,0
    UNION ALL SELECT 44,1037,0
    UNION ALL SELECT 0,41105,0
    UNION ALL SELECT 8,41105,1
    UNION ALL SELECT 36,0,0) AS rule
WHERE @stromkar_missing_loot;

UPDATE gameobject_template SET Data1=247877 WHERE entry=247877 AND Data1=0 AND @stromkar_missing_loot;

COMMIT;
