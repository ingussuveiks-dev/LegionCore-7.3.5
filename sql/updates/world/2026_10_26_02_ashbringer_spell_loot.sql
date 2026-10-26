START TRANSACTION;

SET @ashbringer_spell_loot := EXISTS (SELECT 1 FROM gameobject_template WHERE entry=247318 AND type=3 AND Data1=0
    AND Data0=1691 AND Data15=0 AND Data17=32543 AND Data30=0 AND Data31=0
    AND flags=2113536 AND displayId=34828 AND AIName='' AND ScriptName='go_bs_the_ashbringer')
AND (SELECT COUNT(*) FROM gameobject WHERE id=247318)=1
AND EXISTS (SELECT 1 FROM gameobject WHERE guid=128117 AND id=247318 AND map=1500 AND PhaseId='5942')
AND NOT EXISTS (SELECT 1 FROM spell_loot_template WHERE Entry=199827)
AND NOT EXISTS (SELECT 1 FROM conditions WHERE SourceTypeOrReferenceId=12 AND SourceGroup=199827)
AND NOT EXISTS (SELECT 1 FROM spell_script_names WHERE spell_id=199827);

INSERT INTO spell_loot_template (Entry,Item,Currency,Reference,Chance,QuestRequired,LootMode,GroupId,MinCount,MaxCount)
SELECT 199827,120978,0,0,100,0,1,0,1,1 WHERE @ashbringer_spell_loot;

INSERT INTO conditions (SourceTypeOrReferenceId,SourceGroup,SourceEntry,SourceId,ElseGroup,ConditionTypeOrReference,ConditionTarget,
    ConditionValue1,ConditionValue2,ConditionValue3,NegativeCondition,ErrorTextId,ScriptName,Comment)
SELECT 12,199827,120978,0,variant.else_group,IF(rule.condition_type=0,variant.quest_type,rule.condition_type),0,
    rule.value1,rule.value2,rule.value3,rule.negative,0,'','Ashbringer: native retrieval spell loot for active stage-three Paladins without an owned copy'
FROM (SELECT 0 AS else_group,9 AS quest_type UNION ALL SELECT 1,28) AS variant
CROSS JOIN (SELECT 15 AS condition_type,2 AS value1,0 AS value2,0 AS value3,0 AS negative
    UNION ALL SELECT 22,1500,0,0,0
    UNION ALL SELECT 44,775,3,0,0
    UNION ALL SELECT 0,38376,0,0,0
    UNION ALL SELECT 8,38376,0,0,1
    UNION ALL SELECT 36,0,0,0,0
    UNION ALL SELECT 2,120978,1,1,1) AS rule
WHERE @ashbringer_spell_loot;

COMMIT;
