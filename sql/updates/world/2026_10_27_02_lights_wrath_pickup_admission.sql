-- Promote with the matching physical248572/1583/1065/6..7 admission guard.
START TRANSACTION;
SET @lights_wrath_safe := EXISTS(SELECT 1 FROM gameobject_template WHERE entry=248572
 AND type=3 AND Data0=1691 AND Data1=248572 AND AIName='SmartGameObjectAI' AND ScriptName='go_artifact_acquisition')
 AND (SELECT COUNT(*) FROM gameobject_loot_template WHERE Entry=248572)=1
 AND EXISTS(SELECT 1 FROM gameobject_loot_template WHERE Entry=248572 AND Item=128868
 AND Reference=0 AND Chance=100 AND QuestRequired=0 AND MinCount=1 AND MaxCount=1)
 AND (SELECT COUNT(*) FROM smart_scripts WHERE entryorguid=248572 AND source_type=1)=2
 AND (SELECT COUNT(*) FROM smart_scripts WHERE entryorguid=248572 AND source_type=1
 AND ((id=0 AND link=1 AND event_type=70 AND event_param1=3 AND action_type=85 AND action_param1=208011 AND target_type=7)
 OR(id=1 AND link=0 AND event_type=61 AND action_type=50 AND action_param1=248574 AND target_type=8)))=2
 AND NOT EXISTS(SELECT 1 FROM conditions WHERE
 ((SourceTypeOrReferenceId=4 AND SourceGroup=248572)
 OR(SourceTypeOrReferenceId=17 AND SourceEntry=208011)
 OR(SourceTypeOrReferenceId=22 AND SourceEntry=248572 AND SourceId=1))
 AND COALESCE(Comment,'') NOT LIKE 'Lights Wrath pickup - %'
 AND NOT(SourceTypeOrReferenceId=22 AND SourceGroup=3 AND SourceEntry=248572 AND SourceId=1
 AND ElseGroup=0 AND ConditionTypeOrReference=2 AND ConditionTarget=0 AND ConditionValue1=128868
 AND ConditionValue2=1 AND ConditionValue3=0 AND NegativeCondition=0 AND ErrorTextId=0 AND ScriptName=''));
SET @lights_wrath_guard := IF(@lights_wrath_safe,'DO 0',
 'SIGNAL SQLSTATE ''45000'' SET MESSAGE_TEXT=''Lights Wrath pickup custom/missing native metadata; review before promotion''');
PREPARE lights_wrath_check FROM @lights_wrath_guard;
EXECUTE lights_wrath_check;
DEALLOCATE PREPARE lights_wrath_check;
DELETE FROM conditions WHERE @lights_wrath_safe AND
 ((SourceTypeOrReferenceId=4 AND SourceGroup=248572)
 OR(SourceTypeOrReferenceId=17 AND SourceEntry=208011)
 OR(SourceTypeOrReferenceId=22 AND SourceEntry=248572 AND SourceId=1));
-- Exact native final-stage loot admission also rechecks cached open slots.
INSERT INTO conditions(SourceTypeOrReferenceId,SourceGroup,SourceEntry,SourceId,ElseGroup,
 ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2,ConditionValue3,
 NegativeCondition,ErrorTextId,ScriptName,Comment)
SELECT 4,248572,128868,0,variant.eg,IF(rule.kind=0,variant.quest_type,rule.kind),0,
 rule.v1,rule.v2,rule.v3,rule.negative,0,'','Lights Wrath pickup - native final-step unowned loot'
FROM (SELECT 0 eg,9 quest_type UNION ALL SELECT 1,28) variant
CROSS JOIN (SELECT 15 kind,16 v1,0 v2,0 v3,0 negative
 UNION ALL SELECT 22,1583,0,0,0 UNION ALL SELECT 44,1065,6,0,0
 UNION ALL SELECT 0,41631,0,0,0 UNION ALL SELECT 8,41631,0,0,1
 UNION ALL SELECT 36,0,0,0,0 UNION ALL SELECT 2,128868,1,1,1) rule
WHERE @lights_wrath_safe;
-- The existing bank/shop replay remains native; never grant a new item.
-- Recheck the actual state70 root and deferred action85 spell execution.
INSERT INTO conditions(SourceTypeOrReferenceId,SourceGroup,SourceEntry,SourceId,ElseGroup,
 ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2,ConditionValue3,
 NegativeCondition,ErrorTextId,ScriptName,Comment)
SELECT scope.kind,scope.grp,scope.entry,scope.sid,variant.eg,
 IF(rule.kind=0,variant.quest_type,rule.kind),0,rule.v1,
 IF(rule.kind=44,variant.stage,rule.v2),rule.v3,rule.negative,0,'',
 'Lights Wrath pickup - actual owned native final-step state and cast'
FROM (SELECT 22 kind,1 grp,248572 entry,1 sid UNION ALL SELECT 17,0,208011,0) scope
CROSS JOIN (SELECT 0 eg,9 quest_type,6 stage UNION ALL SELECT 1,28,6
 UNION ALL SELECT 2,9,7 UNION ALL SELECT 3,28,7) variant
CROSS JOIN (SELECT 15 kind,16 v1,0 v2,0 v3,0 negative
 UNION ALL SELECT 22,1583,0,0,0 UNION ALL SELECT 44,1065,6,0,0
 UNION ALL SELECT 0,41631,0,0,0 UNION ALL SELECT 8,41631,0,0,1
 UNION ALL SELECT 36,0,0,0,0 UNION ALL SELECT 2,128868,1,1,0) rule
WHERE @lights_wrath_safe;
COMMIT;
