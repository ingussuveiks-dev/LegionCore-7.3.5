-- Code-coupled: promote only with the verified native-loot Devourer executable.
-- Keep native lock43, scene1257 and existing loot; never create an artifact in C++.
START TRANSACTION;
SET @devourer_pickup_safe := EXISTS (SELECT 1 FROM gameobject_template
 WHERE entry=249347 AND type=3 AND displayId=9806 AND Data0=43 AND Data1 IN(0,249347)
 AND AIName='' AND ScriptName='go_art_devourer')
 AND (SELECT COUNT(*) FROM gameobject_loot_template WHERE Entry=249347)=1
 AND EXISTS(SELECT 1 FROM gameobject_loot_template WHERE Entry=249347 AND Item=128476 AND Reference=0
 AND Chance=100 AND QuestRequired=0 AND MinCount=1 AND MaxCount=1)
 AND NOT EXISTS(SELECT 1 FROM conditions WHERE SourceTypeOrReferenceId=4 AND SourceGroup=249347
 AND COALESCE(Comment,'') NOT LIKE 'Devourer native pickup - %');
UPDATE gameobject_template SET Data1=249347 WHERE @devourer_pickup_safe AND entry=249347;
DELETE FROM conditions WHERE @devourer_pickup_safe AND SourceTypeOrReferenceId=4 AND SourceGroup=249347
 AND Comment LIKE 'Devourer native pickup - %';
INSERT INTO conditions (SourceTypeOrReferenceId,SourceGroup,SourceEntry,SourceId,ElseGroup,ConditionTypeOrReference,ConditionTarget,
 ConditionValue1,ConditionValue2,ConditionValue3,NegativeCondition,ErrorTextId,ScriptName,Comment)
SELECT 4,249347,128476,0,variant.else_group,IF(rule.condition_type=0,variant.quest_type,rule.condition_type),0,
 rule.value1,rule.value2,rule.value3,rule.negative,0,'','Devourer native pickup - active final-step Rogue without an owned copy'
FROM (SELECT 0 AS else_group,9 AS quest_type UNION ALL SELECT 1,28) AS variant
CROSS JOIN (SELECT 15 AS condition_type,8 AS value1,0 AS value2,0 AS value3,0 AS negative
 UNION ALL SELECT 22,1607,0,0,0
 UNION ALL SELECT 44,1078,6,0,0
 UNION ALL SELECT 0,41924,0,0,0
 UNION ALL SELECT 8,41924,0,0,1
 UNION ALL SELECT 36,0,0,0,0
 UNION ALL SELECT 2,128476,1,1,1) AS rule
WHERE @devourer_pickup_safe;
COMMIT;
