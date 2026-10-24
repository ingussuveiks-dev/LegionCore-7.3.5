START TRANSACTION;

SET @xalatath_missing_loot := EXISTS (
    SELECT 1 FROM gameobject_template
    WHERE entry=247690 AND type=3 AND Data0=1691 AND Data1=0 AND Data14=23645
      AND AIName='SmartGameObjectAI' AND ScriptName=''
) AND (SELECT COUNT(*) FROM gameobject_loot_template WHERE Entry=247690)=1
AND EXISTS (
    SELECT 1 FROM gameobject_loot_template WHERE Entry=247690 AND Item=128827
      AND Currency=0 AND Reference=0 AND Chance=100 AND QuestRequired=0
      AND LootMode=1 AND GroupId=0 AND MinCount=1 AND MaxCount=1
) AND (SELECT COUNT(*) FROM smart_scripts WHERE entryorguid=247690 AND source_type=1)=5
AND (SELECT COUNT(*) FROM smart_scripts WHERE entryorguid=247690 AND source_type=1
    AND event_phase_mask=0 AND event_chance=100 AND target_type=7
    AND ((id=0 AND link=1 AND event_type=70 AND event_param1=3 AND event_flags=0 AND action_type=85 AND action_param1=201780 AND action_param2=0)
      OR (id=1 AND link=3 AND event_type=61 AND event_param1=0 AND event_flags=0 AND action_type=205 AND action_param1=92 AND action_param2=48493)
      OR (id=2 AND link=1 AND event_type=64 AND event_param1=3 AND event_flags=1 AND action_type=85 AND action_param1=201780 AND action_param2=0)
      OR (id=3 AND link=4 AND event_type=61 AND event_flags=0 AND action_type=85 AND action_param1=204919 AND action_param2=0)
      OR (id=4 AND link=0 AND event_type=61 AND event_flags=0 AND action_type=85 AND action_param1=201917 AND action_param2=0)))=5
AND NOT EXISTS (SELECT 1 FROM conditions WHERE SourceTypeOrReferenceId=4 AND SourceGroup=247690);

INSERT INTO conditions (SourceTypeOrReferenceId,SourceGroup,SourceEntry,SourceId,ElseGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2,ConditionValue3,NegativeCondition,ErrorTextId,ScriptName,Comment)
SELECT 4,247690,128827,0,variant.else_group,
       IF(rule.condition_type=0,variant.quest_type,rule.condition_type),0,rule.value1,
       IF(rule.condition_type=44,variant.stage,0),0,rule.negative,0,'','Xalatath: active Priest native blade/after-claim stage real loot'
FROM (
    SELECT 0 AS else_group,8 AS stage,9 AS quest_type
    UNION ALL SELECT 1,8,28
    UNION ALL SELECT 2,9,9
    UNION ALL SELECT 3,9,28
) AS variant
CROSS JOIN (
    SELECT 15 AS condition_type,16 AS value1,0 AS negative
    UNION ALL SELECT 22,1539,0
    UNION ALL SELECT 44,991,0
    UNION ALL SELECT 0,40710,0
    UNION ALL SELECT 8,40710,1
) AS rule
WHERE @xalatath_missing_loot;

UPDATE gameobject_template SET Data1=247690
WHERE entry=247690 AND Data1=0 AND @xalatath_missing_loot;

COMMIT;
