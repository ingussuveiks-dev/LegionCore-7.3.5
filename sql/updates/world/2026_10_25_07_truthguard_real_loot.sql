START TRANSACTION;

SET @truthguard_real_loot := EXISTS (SELECT 1 FROM gameobject_template WHERE entry=249420 AND type=3 AND Data1=0
    AND Data0=1691 AND Data15=0 AND Data17=0 AND Data30=0 AND Data31=0 AND flags=2113536 AND displayId=9806 AND AIName='' AND ScriptName='go_sr_truthguard')
AND (SELECT COUNT(*) FROM gameobject_loot_template WHERE Entry=249420)=1
AND EXISTS (SELECT 1 FROM gameobject_loot_template WHERE Entry=249420 AND Item=128866 AND Currency=0 AND Reference=0
    AND Chance=100 AND QuestRequired=0 AND LootMode=1 AND GroupId=0 AND MinCount=1 AND MaxCount=1)
AND (SELECT COUNT(*) FROM gameobject WHERE id=249420)=1
AND EXISTS (SELECT 1 FROM gameobject WHERE guid=140818 AND id=249420 AND map=1495 AND PhaseId='6383')
AND NOT EXISTS (SELECT 1 FROM smart_scripts WHERE entryorguid IN(249420,-140818) AND source_type=1)
AND NOT EXISTS (SELECT 1 FROM conditions WHERE SourceTypeOrReferenceId=4 AND SourceGroup=249420 AND SourceEntry=128866);

UPDATE gameobject_template SET Data1=249420 WHERE entry=249420 AND @truthguard_real_loot;

INSERT INTO conditions (SourceTypeOrReferenceId,SourceGroup,SourceEntry,SourceId,ElseGroup,ConditionTypeOrReference,ConditionTarget,
    ConditionValue1,ConditionValue2,ConditionValue3,NegativeCondition,ErrorTextId,ScriptName,Comment)
SELECT 4,249420,128866,0,variant.else_group,IF(rule.condition_type=0,variant.quest_type,rule.condition_type),0,rule.value1,
    rule.value2,rule.value3,rule.negative,0,'','Truthguard: native final-stage loot for an active eligible Paladin without a carried or bank copy'
FROM (SELECT 0 AS else_group,9 AS quest_type UNION ALL SELECT 1,28) AS variant
CROSS JOIN (SELECT 15 AS condition_type,2 AS value1,0 AS value2,0 AS value3,0 AS negative
    UNION ALL SELECT 22,1495,0,0,0
    UNION ALL SELECT 44,1082,5,0,0
    UNION ALL SELECT 0,42017,0,0,0
    UNION ALL SELECT 8,42017,0,0,1
    UNION ALL SELECT 36,0,0,0,0
    UNION ALL SELECT 2,128866,1,1,1) AS rule
WHERE @truthguard_real_loot;

COMMIT;
