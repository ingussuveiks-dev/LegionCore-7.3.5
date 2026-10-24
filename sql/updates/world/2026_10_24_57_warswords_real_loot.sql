START TRANSACTION;

SET @warswords_real_loot := EXISTS (SELECT 1 FROM gameobject_template WHERE entry=248832 AND type=3 AND Data1=0
    AND Data0=1691 AND Data30=0 AND Data31=1 AND flags=2113536 AND displayId=9806 AND AIName='' AND ScriptName='go_art_warswords')
AND (SELECT COUNT(*) FROM gameobject_loot_template WHERE Entry=248832)=1
AND EXISTS (SELECT 1 FROM gameobject_loot_template WHERE Entry=248832 AND Item=128908 AND Currency=0 AND Reference=0
    AND Chance=100 AND QuestRequired=0 AND LootMode=1 AND GroupId=0 AND MinCount=1 AND MaxCount=1)
AND (SELECT COUNT(*) FROM gameobject WHERE id=248832)=1
AND EXISTS (SELECT 1 FROM gameobject WHERE guid=134150 AND id=248832 AND map=1511 AND PhaseId='7001')
AND NOT EXISTS (SELECT 1 FROM smart_scripts WHERE entryorguid=248832 AND source_type=1)
AND NOT EXISTS (SELECT 1 FROM conditions WHERE SourceTypeOrReferenceId=4 AND SourceGroup=248832 AND SourceEntry=128908);

UPDATE gameobject_template SET Data1=248832 WHERE entry=248832 AND @warswords_real_loot;

INSERT INTO conditions (SourceTypeOrReferenceId,SourceGroup,SourceEntry,SourceId,ElseGroup,ConditionTypeOrReference,ConditionTarget,
    ConditionValue1,ConditionValue2,ConditionValue3,NegativeCondition,ErrorTextId,ScriptName,Comment)
SELECT 4,248832,128908,0,variant.else_group,IF(rule.condition_type=0,variant.quest_type,rule.condition_type),0,rule.value1,
    rule.value2,rule.value3,rule.negative,0,'','Warswords: real final-stage loot for eligible active Warrior without an owned copy'
FROM (SELECT 0 AS else_group,9 AS quest_type UNION ALL SELECT 1,28) AS variant
CROSS JOIN (SELECT 15 AS condition_type,1 AS value1,0 AS value2,0 AS value3,0 AS negative
    UNION ALL SELECT 22,1511,0,0,0
    UNION ALL SELECT 44,944,6,0,0
    UNION ALL SELECT 0,40043,0,0,0
    UNION ALL SELECT 8,40043,0,0,1
    UNION ALL SELECT 36,0,0,0,0
    UNION ALL SELECT 2,128908,1,1,1) AS rule
WHERE @warswords_real_loot;

COMMIT;
