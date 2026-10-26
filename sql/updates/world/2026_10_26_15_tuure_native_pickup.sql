-- Code-coupled: promote only with the verified native-loot Tuure executable.
-- Keep native lock1691, scene1265 and existing loot; never create an artifact in C++.
START TRANSACTION;
SET @tuure_pickup_safe := EXISTS (SELECT 1 FROM gameobject_template
 WHERE entry=249705 AND type=3 AND displayId=9806 AND Data0=1691 AND Data1 IN(0,249705)
 AND AIName='' AND ScriptName='go_art_tuure')
 AND (SELECT COUNT(*) FROM gameobject_loot_template WHERE Entry=249705)=1
 AND EXISTS(SELECT 1 FROM gameobject_loot_template WHERE Entry=249705 AND Item=128825 AND Reference=0
 AND Chance=100 AND QuestRequired=0 AND MinCount=1 AND MaxCount=1)
 AND NOT EXISTS(SELECT 1 FROM conditions WHERE SourceTypeOrReferenceId=4 AND SourceGroup=249705
 AND COALESCE(Comment,'') NOT LIKE 'Tuure native pickup - %')
 AND EXISTS(SELECT 1 FROM spell_scene WHERE MiscValue=1265 AND SceneScriptPackageID=1565
 AND PlaybackFlags=21 AND CustomDuration=0 AND ScriptName='scene_tuure_pickup')
 AND NOT EXISTS(SELECT 1 FROM spell_scene_event WHERE MiscValue=1265)
 AND EXISTS(SELECT 1 FROM spell_target_position WHERE id=210818 AND target_map=1220
 AND ABS(target_position_x+828.85)<0.001 AND ABS(target_position_y-4372.0)<0.001
 AND ABS(target_position_z-738.64)<0.001 AND ABS(target_orientation-1.86768)<0.001)
 AND NOT EXISTS(SELECT 1 FROM spell_script_names WHERE ABS(spell_id) IN(210733,210734,210818)
 AND NOT((spell_id IN(210733,210734) AND ScriptName='spell_tuure_pickup_cast')
 OR (spell_id=210818 AND ScriptName='spell_tuure_departure')));
SET @tuure_pickup_guard := IF(@tuure_pickup_safe,'DO 0',
 'SIGNAL SQLSTATE ''45000'' SET MESSAGE_TEXT=''Tuure native pickup custom/missing metadata; review before promotion''');
PREPARE tuure_pickup_check FROM @tuure_pickup_guard;
EXECUTE tuure_pickup_check;
DEALLOCATE PREPARE tuure_pickup_check;
UPDATE gameobject_template SET Data1=249705 WHERE @tuure_pickup_safe AND entry=249705;
DELETE FROM conditions WHERE @tuure_pickup_safe AND SourceTypeOrReferenceId=4 AND SourceGroup=249705
 AND Comment LIKE 'Tuure native pickup - %';
INSERT INTO conditions (SourceTypeOrReferenceId,SourceGroup,SourceEntry,SourceId,ElseGroup,ConditionTypeOrReference,ConditionTarget,
 ConditionValue1,ConditionValue2,ConditionValue3,NegativeCondition,ErrorTextId,ScriptName,Comment)
SELECT 4,249705,128825,0,variant.else_group,IF(rule.condition_type=0,variant.quest_type,rule.condition_type),0,
 rule.value1,rule.value2,rule.value3,rule.negative,0,'','Tuure native pickup - active final-step Priest without an owned copy'
FROM (SELECT 0 AS else_group,9 AS quest_type UNION ALL SELECT 1,28) AS variant
CROSS JOIN (SELECT 15 AS condition_type,16 AS value1,0 AS value2,0 AS value3,0 AS negative
 UNION ALL SELECT 22,1489,0,0,0
 UNION ALL SELECT 44,1085,6,0,0
 UNION ALL SELECT 0,41993,0,0,0
 UNION ALL SELECT 8,41993,0,0,1
 UNION ALL SELECT 36,0,0,0,0
 UNION ALL SELECT 2,128825,1,1,1) AS rule
WHERE @tuure_pickup_safe;
INSERT IGNORE INTO spell_script_names(spell_id,ScriptName)
SELECT 210733,'spell_tuure_pickup_cast' WHERE @tuure_pickup_safe
UNION ALL SELECT 210734,'spell_tuure_pickup_cast' WHERE @tuure_pickup_safe
UNION ALL SELECT 210818,'spell_tuure_departure' WHERE @tuure_pickup_safe;
COMMIT;
