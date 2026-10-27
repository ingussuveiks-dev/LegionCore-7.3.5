-- CODE-COUPLED: promote matching Kingslayers loot/equip handlers first.
SET @kingslayers_previous_concat := @@SESSION.group_concat_max_len;
SET SESSION group_concat_max_len=GREATEST(@@SESSION.group_concat_max_len,1048576);
START TRANSACTION;
SET @kingslayers_conditions := (SELECT SHA2(GROUP_CONCAT(CONCAT_WS('|',SourceTypeOrReferenceId,SourceGroup,SourceEntry,SourceId,ElseGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2,ConditionValue3,NegativeCondition,ErrorTextId,ScriptName,Comment) ORDER BY SourceTypeOrReferenceId,SourceGroup,SourceEntry,SourceId,ElseGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2,ConditionValue3,NegativeCondition,ErrorTextId,ScriptName,Comment SEPARATOR '\n'),256) FROM conditions WHERE (SourceTypeOrReferenceId=4 AND SourceGroup=251107) OR(SourceTypeOrReferenceId=17 AND SourceEntry IN(215112,215113,215114)));
SET @kingslayers_count := (SELECT COUNT(*) FROM conditions WHERE (SourceTypeOrReferenceId=4 AND SourceGroup=251107) OR(SourceTypeOrReferenceId=17 AND SourceEntry IN(215112,215113,215114)));
SET @kingslayers_safe := (@kingslayers_count=0 OR(@kingslayers_count=196 AND @kingslayers_conditions='633f0a074dc50580d8a4de86cbbf96f7b9082c8b7b188f44e0e7a28abdfc9aad'))
 AND (SELECT COUNT(*) FROM gameobject_template WHERE entry=251107 AND type=3 AND displayId=9806 AND Data0=43 AND Data1 IN(0,251107) AND AIName='' AND ScriptName='go_the_kingslayers')=1
 AND (SELECT COUNT(*) FROM gameobject_loot_template WHERE Entry=251107)=1
 AND (SELECT COUNT(*) FROM gameobject_loot_template WHERE Entry=251107 AND Item=128870 AND Reference=0 AND Chance=100 AND QuestRequired=0 AND MinCount=1 AND MaxCount=1)=1;
SET @kingslayers_guard := IF(@kingslayers_safe,'DO 0','SIGNAL SQLSTATE ''45000'' SET MESSAGE_TEXT=''Kingslayers native template/loot/conditions differ; review before promotion''');
PREPARE kingslayers_check FROM @kingslayers_guard;EXECUTE kingslayers_check;DEALLOCATE PREPARE kingslayers_check;
UPDATE gameobject_template SET Data1=251107 WHERE @kingslayers_safe AND entry=251107;
DELETE FROM conditions WHERE @kingslayers_safe AND ((SourceTypeOrReferenceId=4 AND SourceGroup=251107) OR(SourceTypeOrReferenceId=17 AND SourceEntry IN(215112,215113,215114)));
INSERT INTO conditions(SourceTypeOrReferenceId,SourceGroup,SourceEntry,SourceId,ElseGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2,ConditionValue3,NegativeCondition,ErrorTextId,ScriptName,Comment)
SELECT SourceTypeOrReferenceId,SourceGroup,SourceEntry,SourceId,ElseGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2,ConditionValue3,NegativeCondition,ErrorTextId,ScriptName,Comment FROM (
SELECT 4 AS SourceTypeOrReferenceId,251107 AS SourceGroup,128870 AS SourceEntry,0 AS SourceId,0 AS ElseGroup,2 AS ConditionTypeOrReference,0 AS ConditionTarget,128870 AS ConditionValue1,1 AS ConditionValue2,1 AS ConditionValue3,1 AS NegativeCondition,0 AS ErrorTextId,'' AS ScriptName,'Kingslayers native pickup - native loot admission' AS Comment
UNION ALL
SELECT 4,251107,128870,0,0,8,0,42504,0,0,1,0,'','Kingslayers native pickup - native loot admission'
UNION ALL
SELECT 4,251107,128870,0,0,9,0,42504,0,0,0,0,'','Kingslayers native pickup - native loot admission'
UNION ALL
SELECT 4,251107,128870,0,0,15,0,8,0,0,0,0,'','Kingslayers native pickup - native loot admission'
UNION ALL
SELECT 4,251107,128870,0,0,22,0,1620,0,0,0,0,'','Kingslayers native pickup - native loot admission'
UNION ALL
SELECT 4,251107,128870,0,0,36,0,0,0,0,0,0,'','Kingslayers native pickup - native loot admission'
UNION ALL
SELECT 4,251107,128870,0,0,44,0,1123,8,0,0,0,'','Kingslayers native pickup - native loot admission'
UNION ALL
SELECT 4,251107,128870,0,1,2,0,128870,1,1,1,0,'','Kingslayers native pickup - native loot admission'
UNION ALL
SELECT 4,251107,128870,0,1,8,0,42504,0,0,1,0,'','Kingslayers native pickup - native loot admission'
UNION ALL
SELECT 4,251107,128870,0,1,15,0,8,0,0,0,0,'','Kingslayers native pickup - native loot admission'
UNION ALL
SELECT 4,251107,128870,0,1,22,0,1620,0,0,0,0,'','Kingslayers native pickup - native loot admission'
UNION ALL
SELECT 4,251107,128870,0,1,28,0,42504,0,0,0,0,'','Kingslayers native pickup - native loot admission'
UNION ALL
SELECT 4,251107,128870,0,1,36,0,0,0,0,0,0,'','Kingslayers native pickup - native loot admission'
UNION ALL
SELECT 4,251107,128870,0,1,44,0,1123,8,0,0,0,'','Kingslayers native pickup - native loot admission'
UNION ALL
SELECT 4,251107,128870,0,2,2,0,128870,1,1,1,0,'','Kingslayers native pickup - native loot admission'
UNION ALL
SELECT 4,251107,128870,0,2,8,0,42627,0,0,1,0,'','Kingslayers native pickup - native loot admission'
UNION ALL
SELECT 4,251107,128870,0,2,9,0,42627,0,0,0,0,'','Kingslayers native pickup - native loot admission'
UNION ALL
SELECT 4,251107,128870,0,2,15,0,8,0,0,0,0,'','Kingslayers native pickup - native loot admission'
UNION ALL
SELECT 4,251107,128870,0,2,22,0,1620,0,0,0,0,'','Kingslayers native pickup - native loot admission'
UNION ALL
SELECT 4,251107,128870,0,2,36,0,0,0,0,0,0,'','Kingslayers native pickup - native loot admission'
UNION ALL
SELECT 4,251107,128870,0,2,44,0,1123,8,0,0,0,'','Kingslayers native pickup - native loot admission'
UNION ALL
SELECT 4,251107,128870,0,3,2,0,128870,1,1,1,0,'','Kingslayers native pickup - native loot admission'
UNION ALL
SELECT 4,251107,128870,0,3,8,0,42627,0,0,1,0,'','Kingslayers native pickup - native loot admission'
UNION ALL
SELECT 4,251107,128870,0,3,15,0,8,0,0,0,0,'','Kingslayers native pickup - native loot admission'
UNION ALL
SELECT 4,251107,128870,0,3,22,0,1620,0,0,0,0,'','Kingslayers native pickup - native loot admission'
UNION ALL
SELECT 4,251107,128870,0,3,28,0,42627,0,0,0,0,'','Kingslayers native pickup - native loot admission'
UNION ALL
SELECT 4,251107,128870,0,3,36,0,0,0,0,0,0,'','Kingslayers native pickup - native loot admission'
UNION ALL
SELECT 4,251107,128870,0,3,44,0,1123,8,0,0,0,'','Kingslayers native pickup - native loot admission'
UNION ALL
SELECT 17,0,215112,0,0,2,0,128870,1,0,0,0,'','Kingslayers native pickup - owned final-stage Rogue'
UNION ALL
SELECT 17,0,215112,0,0,8,0,42504,0,0,1,0,'','Kingslayers native pickup - owned final-stage Rogue'
UNION ALL
SELECT 17,0,215112,0,0,9,0,42504,0,0,0,0,'','Kingslayers native pickup - owned final-stage Rogue'
UNION ALL
SELECT 17,0,215112,0,0,15,0,8,0,0,0,0,'','Kingslayers native pickup - owned final-stage Rogue'
UNION ALL
SELECT 17,0,215112,0,0,22,0,1620,0,0,0,0,'','Kingslayers native pickup - owned final-stage Rogue'
UNION ALL
SELECT 17,0,215112,0,0,36,0,0,0,0,0,0,'','Kingslayers native pickup - owned final-stage Rogue'
UNION ALL
SELECT 17,0,215112,0,0,44,0,1123,8,0,0,0,'','Kingslayers native pickup - owned final-stage Rogue'
UNION ALL
SELECT 17,0,215112,0,1,2,0,128870,1,0,0,0,'','Kingslayers native pickup - owned final-stage Rogue'
UNION ALL
SELECT 17,0,215112,0,1,8,0,42504,0,0,1,0,'','Kingslayers native pickup - owned final-stage Rogue'
UNION ALL
SELECT 17,0,215112,0,1,9,0,42504,0,0,0,0,'','Kingslayers native pickup - owned final-stage Rogue'
UNION ALL
SELECT 17,0,215112,0,1,15,0,8,0,0,0,0,'','Kingslayers native pickup - owned final-stage Rogue'
UNION ALL
SELECT 17,0,215112,0,1,22,0,1620,0,0,0,0,'','Kingslayers native pickup - owned final-stage Rogue'
UNION ALL
SELECT 17,0,215112,0,1,36,0,0,0,0,0,0,'','Kingslayers native pickup - owned final-stage Rogue'
UNION ALL
SELECT 17,0,215112,0,1,44,0,1123,9,0,0,0,'','Kingslayers native pickup - owned final-stage Rogue'
UNION ALL
SELECT 17,0,215112,0,2,2,0,128870,1,0,0,0,'','Kingslayers native pickup - owned final-stage Rogue'
UNION ALL
SELECT 17,0,215112,0,2,8,0,42504,0,0,1,0,'','Kingslayers native pickup - owned final-stage Rogue'
UNION ALL
SELECT 17,0,215112,0,2,15,0,8,0,0,0,0,'','Kingslayers native pickup - owned final-stage Rogue'
UNION ALL
SELECT 17,0,215112,0,2,22,0,1620,0,0,0,0,'','Kingslayers native pickup - owned final-stage Rogue'
UNION ALL
SELECT 17,0,215112,0,2,28,0,42504,0,0,0,0,'','Kingslayers native pickup - owned final-stage Rogue'
UNION ALL
SELECT 17,0,215112,0,2,36,0,0,0,0,0,0,'','Kingslayers native pickup - owned final-stage Rogue'
UNION ALL
SELECT 17,0,215112,0,2,44,0,1123,8,0,0,0,'','Kingslayers native pickup - owned final-stage Rogue'
UNION ALL
SELECT 17,0,215112,0,3,2,0,128870,1,0,0,0,'','Kingslayers native pickup - owned final-stage Rogue'
UNION ALL
SELECT 17,0,215112,0,3,8,0,42504,0,0,1,0,'','Kingslayers native pickup - owned final-stage Rogue'
UNION ALL
SELECT 17,0,215112,0,3,15,0,8,0,0,0,0,'','Kingslayers native pickup - owned final-stage Rogue'
UNION ALL
SELECT 17,0,215112,0,3,22,0,1620,0,0,0,0,'','Kingslayers native pickup - owned final-stage Rogue'
UNION ALL
SELECT 17,0,215112,0,3,28,0,42504,0,0,0,0,'','Kingslayers native pickup - owned final-stage Rogue'
UNION ALL
SELECT 17,0,215112,0,3,36,0,0,0,0,0,0,'','Kingslayers native pickup - owned final-stage Rogue'
UNION ALL
SELECT 17,0,215112,0,3,44,0,1123,9,0,0,0,'','Kingslayers native pickup - owned final-stage Rogue'
UNION ALL
SELECT 17,0,215112,0,4,2,0,128870,1,0,0,0,'','Kingslayers native pickup - owned final-stage Rogue'
UNION ALL
SELECT 17,0,215112,0,4,8,0,42627,0,0,1,0,'','Kingslayers native pickup - owned final-stage Rogue'
UNION ALL
SELECT 17,0,215112,0,4,9,0,42627,0,0,0,0,'','Kingslayers native pickup - owned final-stage Rogue'
UNION ALL
SELECT 17,0,215112,0,4,15,0,8,0,0,0,0,'','Kingslayers native pickup - owned final-stage Rogue'
UNION ALL
SELECT 17,0,215112,0,4,22,0,1620,0,0,0,0,'','Kingslayers native pickup - owned final-stage Rogue'
UNION ALL
SELECT 17,0,215112,0,4,36,0,0,0,0,0,0,'','Kingslayers native pickup - owned final-stage Rogue'
UNION ALL
SELECT 17,0,215112,0,4,44,0,1123,8,0,0,0,'','Kingslayers native pickup - owned final-stage Rogue'
UNION ALL
SELECT 17,0,215112,0,5,2,0,128870,1,0,0,0,'','Kingslayers native pickup - owned final-stage Rogue'
UNION ALL
SELECT 17,0,215112,0,5,8,0,42627,0,0,1,0,'','Kingslayers native pickup - owned final-stage Rogue'
UNION ALL
SELECT 17,0,215112,0,5,9,0,42627,0,0,0,0,'','Kingslayers native pickup - owned final-stage Rogue'
UNION ALL
SELECT 17,0,215112,0,5,15,0,8,0,0,0,0,'','Kingslayers native pickup - owned final-stage Rogue'
UNION ALL
SELECT 17,0,215112,0,5,22,0,1620,0,0,0,0,'','Kingslayers native pickup - owned final-stage Rogue'
UNION ALL
SELECT 17,0,215112,0,5,36,0,0,0,0,0,0,'','Kingslayers native pickup - owned final-stage Rogue'
UNION ALL
SELECT 17,0,215112,0,5,44,0,1123,9,0,0,0,'','Kingslayers native pickup - owned final-stage Rogue'
UNION ALL
SELECT 17,0,215112,0,6,2,0,128870,1,0,0,0,'','Kingslayers native pickup - owned final-stage Rogue'
UNION ALL
SELECT 17,0,215112,0,6,8,0,42627,0,0,1,0,'','Kingslayers native pickup - owned final-stage Rogue'
UNION ALL
SELECT 17,0,215112,0,6,15,0,8,0,0,0,0,'','Kingslayers native pickup - owned final-stage Rogue'
UNION ALL
SELECT 17,0,215112,0,6,22,0,1620,0,0,0,0,'','Kingslayers native pickup - owned final-stage Rogue'
UNION ALL
SELECT 17,0,215112,0,6,28,0,42627,0,0,0,0,'','Kingslayers native pickup - owned final-stage Rogue'
UNION ALL
SELECT 17,0,215112,0,6,36,0,0,0,0,0,0,'','Kingslayers native pickup - owned final-stage Rogue'
UNION ALL
SELECT 17,0,215112,0,6,44,0,1123,8,0,0,0,'','Kingslayers native pickup - owned final-stage Rogue'
UNION ALL
SELECT 17,0,215112,0,7,2,0,128870,1,0,0,0,'','Kingslayers native pickup - owned final-stage Rogue'
UNION ALL
SELECT 17,0,215112,0,7,8,0,42627,0,0,1,0,'','Kingslayers native pickup - owned final-stage Rogue'
UNION ALL
SELECT 17,0,215112,0,7,15,0,8,0,0,0,0,'','Kingslayers native pickup - owned final-stage Rogue'
UNION ALL
SELECT 17,0,215112,0,7,22,0,1620,0,0,0,0,'','Kingslayers native pickup - owned final-stage Rogue'
UNION ALL
SELECT 17,0,215112,0,7,28,0,42627,0,0,0,0,'','Kingslayers native pickup - owned final-stage Rogue'
UNION ALL
SELECT 17,0,215112,0,7,36,0,0,0,0,0,0,'','Kingslayers native pickup - owned final-stage Rogue'
UNION ALL
SELECT 17,0,215112,0,7,44,0,1123,9,0,0,0,'','Kingslayers native pickup - owned final-stage Rogue'
UNION ALL
SELECT 17,0,215113,0,0,2,0,128870,1,0,0,0,'','Kingslayers native pickup - owned final-stage Rogue'
UNION ALL
SELECT 17,0,215113,0,0,8,0,42504,0,0,1,0,'','Kingslayers native pickup - owned final-stage Rogue'
UNION ALL
SELECT 17,0,215113,0,0,9,0,42504,0,0,0,0,'','Kingslayers native pickup - owned final-stage Rogue'
UNION ALL
SELECT 17,0,215113,0,0,15,0,8,0,0,0,0,'','Kingslayers native pickup - owned final-stage Rogue'
UNION ALL
SELECT 17,0,215113,0,0,22,0,1620,0,0,0,0,'','Kingslayers native pickup - owned final-stage Rogue'
UNION ALL
SELECT 17,0,215113,0,0,36,0,0,0,0,0,0,'','Kingslayers native pickup - owned final-stage Rogue'
UNION ALL
SELECT 17,0,215113,0,0,44,0,1123,8,0,0,0,'','Kingslayers native pickup - owned final-stage Rogue'
UNION ALL
SELECT 17,0,215113,0,1,2,0,128870,1,0,0,0,'','Kingslayers native pickup - owned final-stage Rogue'
UNION ALL
SELECT 17,0,215113,0,1,8,0,42504,0,0,1,0,'','Kingslayers native pickup - owned final-stage Rogue'
UNION ALL
SELECT 17,0,215113,0,1,9,0,42504,0,0,0,0,'','Kingslayers native pickup - owned final-stage Rogue'
UNION ALL
SELECT 17,0,215113,0,1,15,0,8,0,0,0,0,'','Kingslayers native pickup - owned final-stage Rogue'
UNION ALL
SELECT 17,0,215113,0,1,22,0,1620,0,0,0,0,'','Kingslayers native pickup - owned final-stage Rogue'
UNION ALL
SELECT 17,0,215113,0,1,36,0,0,0,0,0,0,'','Kingslayers native pickup - owned final-stage Rogue'
UNION ALL
SELECT 17,0,215113,0,1,44,0,1123,9,0,0,0,'','Kingslayers native pickup - owned final-stage Rogue'
UNION ALL
SELECT 17,0,215113,0,2,2,0,128870,1,0,0,0,'','Kingslayers native pickup - owned final-stage Rogue'
UNION ALL
SELECT 17,0,215113,0,2,8,0,42504,0,0,1,0,'','Kingslayers native pickup - owned final-stage Rogue'
UNION ALL
SELECT 17,0,215113,0,2,15,0,8,0,0,0,0,'','Kingslayers native pickup - owned final-stage Rogue'
UNION ALL
SELECT 17,0,215113,0,2,22,0,1620,0,0,0,0,'','Kingslayers native pickup - owned final-stage Rogue'
UNION ALL
SELECT 17,0,215113,0,2,28,0,42504,0,0,0,0,'','Kingslayers native pickup - owned final-stage Rogue'
UNION ALL
SELECT 17,0,215113,0,2,36,0,0,0,0,0,0,'','Kingslayers native pickup - owned final-stage Rogue'
UNION ALL
SELECT 17,0,215113,0,2,44,0,1123,8,0,0,0,'','Kingslayers native pickup - owned final-stage Rogue'
UNION ALL
SELECT 17,0,215113,0,3,2,0,128870,1,0,0,0,'','Kingslayers native pickup - owned final-stage Rogue'
UNION ALL
SELECT 17,0,215113,0,3,8,0,42504,0,0,1,0,'','Kingslayers native pickup - owned final-stage Rogue'
UNION ALL
SELECT 17,0,215113,0,3,15,0,8,0,0,0,0,'','Kingslayers native pickup - owned final-stage Rogue'
UNION ALL
SELECT 17,0,215113,0,3,22,0,1620,0,0,0,0,'','Kingslayers native pickup - owned final-stage Rogue'
UNION ALL
SELECT 17,0,215113,0,3,28,0,42504,0,0,0,0,'','Kingslayers native pickup - owned final-stage Rogue'
UNION ALL
SELECT 17,0,215113,0,3,36,0,0,0,0,0,0,'','Kingslayers native pickup - owned final-stage Rogue'
UNION ALL
SELECT 17,0,215113,0,3,44,0,1123,9,0,0,0,'','Kingslayers native pickup - owned final-stage Rogue'
UNION ALL
SELECT 17,0,215113,0,4,2,0,128870,1,0,0,0,'','Kingslayers native pickup - owned final-stage Rogue'
UNION ALL
SELECT 17,0,215113,0,4,8,0,42627,0,0,1,0,'','Kingslayers native pickup - owned final-stage Rogue'
UNION ALL
SELECT 17,0,215113,0,4,9,0,42627,0,0,0,0,'','Kingslayers native pickup - owned final-stage Rogue'
UNION ALL
SELECT 17,0,215113,0,4,15,0,8,0,0,0,0,'','Kingslayers native pickup - owned final-stage Rogue'
UNION ALL
SELECT 17,0,215113,0,4,22,0,1620,0,0,0,0,'','Kingslayers native pickup - owned final-stage Rogue'
UNION ALL
SELECT 17,0,215113,0,4,36,0,0,0,0,0,0,'','Kingslayers native pickup - owned final-stage Rogue'
UNION ALL
SELECT 17,0,215113,0,4,44,0,1123,8,0,0,0,'','Kingslayers native pickup - owned final-stage Rogue'
UNION ALL
SELECT 17,0,215113,0,5,2,0,128870,1,0,0,0,'','Kingslayers native pickup - owned final-stage Rogue'
UNION ALL
SELECT 17,0,215113,0,5,8,0,42627,0,0,1,0,'','Kingslayers native pickup - owned final-stage Rogue'
UNION ALL
SELECT 17,0,215113,0,5,9,0,42627,0,0,0,0,'','Kingslayers native pickup - owned final-stage Rogue'
UNION ALL
SELECT 17,0,215113,0,5,15,0,8,0,0,0,0,'','Kingslayers native pickup - owned final-stage Rogue'
UNION ALL
SELECT 17,0,215113,0,5,22,0,1620,0,0,0,0,'','Kingslayers native pickup - owned final-stage Rogue'
UNION ALL
SELECT 17,0,215113,0,5,36,0,0,0,0,0,0,'','Kingslayers native pickup - owned final-stage Rogue'
UNION ALL
SELECT 17,0,215113,0,5,44,0,1123,9,0,0,0,'','Kingslayers native pickup - owned final-stage Rogue'
UNION ALL
SELECT 17,0,215113,0,6,2,0,128870,1,0,0,0,'','Kingslayers native pickup - owned final-stage Rogue'
UNION ALL
SELECT 17,0,215113,0,6,8,0,42627,0,0,1,0,'','Kingslayers native pickup - owned final-stage Rogue'
UNION ALL
SELECT 17,0,215113,0,6,15,0,8,0,0,0,0,'','Kingslayers native pickup - owned final-stage Rogue'
UNION ALL
SELECT 17,0,215113,0,6,22,0,1620,0,0,0,0,'','Kingslayers native pickup - owned final-stage Rogue'
UNION ALL
SELECT 17,0,215113,0,6,28,0,42627,0,0,0,0,'','Kingslayers native pickup - owned final-stage Rogue'
UNION ALL
SELECT 17,0,215113,0,6,36,0,0,0,0,0,0,'','Kingslayers native pickup - owned final-stage Rogue'
UNION ALL
SELECT 17,0,215113,0,6,44,0,1123,8,0,0,0,'','Kingslayers native pickup - owned final-stage Rogue'
UNION ALL
SELECT 17,0,215113,0,7,2,0,128870,1,0,0,0,'','Kingslayers native pickup - owned final-stage Rogue'
UNION ALL
SELECT 17,0,215113,0,7,8,0,42627,0,0,1,0,'','Kingslayers native pickup - owned final-stage Rogue'
UNION ALL
SELECT 17,0,215113,0,7,15,0,8,0,0,0,0,'','Kingslayers native pickup - owned final-stage Rogue'
UNION ALL
SELECT 17,0,215113,0,7,22,0,1620,0,0,0,0,'','Kingslayers native pickup - owned final-stage Rogue'
UNION ALL
SELECT 17,0,215113,0,7,28,0,42627,0,0,0,0,'','Kingslayers native pickup - owned final-stage Rogue'
UNION ALL
SELECT 17,0,215113,0,7,36,0,0,0,0,0,0,'','Kingslayers native pickup - owned final-stage Rogue'
UNION ALL
SELECT 17,0,215113,0,7,44,0,1123,9,0,0,0,'','Kingslayers native pickup - owned final-stage Rogue'
UNION ALL
SELECT 17,0,215114,0,0,2,0,128870,1,0,0,0,'','Kingslayers native pickup - owned final-stage Rogue'
UNION ALL
SELECT 17,0,215114,0,0,8,0,42504,0,0,1,0,'','Kingslayers native pickup - owned final-stage Rogue'
UNION ALL
SELECT 17,0,215114,0,0,9,0,42504,0,0,0,0,'','Kingslayers native pickup - owned final-stage Rogue'
UNION ALL
SELECT 17,0,215114,0,0,15,0,8,0,0,0,0,'','Kingslayers native pickup - owned final-stage Rogue'
UNION ALL
SELECT 17,0,215114,0,0,22,0,1620,0,0,0,0,'','Kingslayers native pickup - owned final-stage Rogue'
UNION ALL
SELECT 17,0,215114,0,0,36,0,0,0,0,0,0,'','Kingslayers native pickup - owned final-stage Rogue'
UNION ALL
SELECT 17,0,215114,0,0,44,0,1123,8,0,0,0,'','Kingslayers native pickup - owned final-stage Rogue'
UNION ALL
SELECT 17,0,215114,0,1,2,0,128870,1,0,0,0,'','Kingslayers native pickup - owned final-stage Rogue'
UNION ALL
SELECT 17,0,215114,0,1,8,0,42504,0,0,1,0,'','Kingslayers native pickup - owned final-stage Rogue'
UNION ALL
SELECT 17,0,215114,0,1,9,0,42504,0,0,0,0,'','Kingslayers native pickup - owned final-stage Rogue'
UNION ALL
SELECT 17,0,215114,0,1,15,0,8,0,0,0,0,'','Kingslayers native pickup - owned final-stage Rogue'
UNION ALL
SELECT 17,0,215114,0,1,22,0,1620,0,0,0,0,'','Kingslayers native pickup - owned final-stage Rogue'
UNION ALL
SELECT 17,0,215114,0,1,36,0,0,0,0,0,0,'','Kingslayers native pickup - owned final-stage Rogue'
UNION ALL
SELECT 17,0,215114,0,1,44,0,1123,9,0,0,0,'','Kingslayers native pickup - owned final-stage Rogue'
UNION ALL
SELECT 17,0,215114,0,2,2,0,128870,1,0,0,0,'','Kingslayers native pickup - owned final-stage Rogue'
UNION ALL
SELECT 17,0,215114,0,2,8,0,42504,0,0,1,0,'','Kingslayers native pickup - owned final-stage Rogue'
UNION ALL
SELECT 17,0,215114,0,2,15,0,8,0,0,0,0,'','Kingslayers native pickup - owned final-stage Rogue'
UNION ALL
SELECT 17,0,215114,0,2,22,0,1620,0,0,0,0,'','Kingslayers native pickup - owned final-stage Rogue'
UNION ALL
SELECT 17,0,215114,0,2,28,0,42504,0,0,0,0,'','Kingslayers native pickup - owned final-stage Rogue'
UNION ALL
SELECT 17,0,215114,0,2,36,0,0,0,0,0,0,'','Kingslayers native pickup - owned final-stage Rogue'
UNION ALL
SELECT 17,0,215114,0,2,44,0,1123,8,0,0,0,'','Kingslayers native pickup - owned final-stage Rogue'
UNION ALL
SELECT 17,0,215114,0,3,2,0,128870,1,0,0,0,'','Kingslayers native pickup - owned final-stage Rogue'
UNION ALL
SELECT 17,0,215114,0,3,8,0,42504,0,0,1,0,'','Kingslayers native pickup - owned final-stage Rogue'
UNION ALL
SELECT 17,0,215114,0,3,15,0,8,0,0,0,0,'','Kingslayers native pickup - owned final-stage Rogue'
UNION ALL
SELECT 17,0,215114,0,3,22,0,1620,0,0,0,0,'','Kingslayers native pickup - owned final-stage Rogue'
UNION ALL
SELECT 17,0,215114,0,3,28,0,42504,0,0,0,0,'','Kingslayers native pickup - owned final-stage Rogue'
UNION ALL
SELECT 17,0,215114,0,3,36,0,0,0,0,0,0,'','Kingslayers native pickup - owned final-stage Rogue'
UNION ALL
SELECT 17,0,215114,0,3,44,0,1123,9,0,0,0,'','Kingslayers native pickup - owned final-stage Rogue'
UNION ALL
SELECT 17,0,215114,0,4,2,0,128870,1,0,0,0,'','Kingslayers native pickup - owned final-stage Rogue'
UNION ALL
SELECT 17,0,215114,0,4,8,0,42627,0,0,1,0,'','Kingslayers native pickup - owned final-stage Rogue'
UNION ALL
SELECT 17,0,215114,0,4,9,0,42627,0,0,0,0,'','Kingslayers native pickup - owned final-stage Rogue'
UNION ALL
SELECT 17,0,215114,0,4,15,0,8,0,0,0,0,'','Kingslayers native pickup - owned final-stage Rogue'
UNION ALL
SELECT 17,0,215114,0,4,22,0,1620,0,0,0,0,'','Kingslayers native pickup - owned final-stage Rogue'
UNION ALL
SELECT 17,0,215114,0,4,36,0,0,0,0,0,0,'','Kingslayers native pickup - owned final-stage Rogue'
UNION ALL
SELECT 17,0,215114,0,4,44,0,1123,8,0,0,0,'','Kingslayers native pickup - owned final-stage Rogue'
UNION ALL
SELECT 17,0,215114,0,5,2,0,128870,1,0,0,0,'','Kingslayers native pickup - owned final-stage Rogue'
UNION ALL
SELECT 17,0,215114,0,5,8,0,42627,0,0,1,0,'','Kingslayers native pickup - owned final-stage Rogue'
UNION ALL
SELECT 17,0,215114,0,5,9,0,42627,0,0,0,0,'','Kingslayers native pickup - owned final-stage Rogue'
UNION ALL
SELECT 17,0,215114,0,5,15,0,8,0,0,0,0,'','Kingslayers native pickup - owned final-stage Rogue'
UNION ALL
SELECT 17,0,215114,0,5,22,0,1620,0,0,0,0,'','Kingslayers native pickup - owned final-stage Rogue'
UNION ALL
SELECT 17,0,215114,0,5,36,0,0,0,0,0,0,'','Kingslayers native pickup - owned final-stage Rogue'
UNION ALL
SELECT 17,0,215114,0,5,44,0,1123,9,0,0,0,'','Kingslayers native pickup - owned final-stage Rogue'
UNION ALL
SELECT 17,0,215114,0,6,2,0,128870,1,0,0,0,'','Kingslayers native pickup - owned final-stage Rogue'
UNION ALL
SELECT 17,0,215114,0,6,8,0,42627,0,0,1,0,'','Kingslayers native pickup - owned final-stage Rogue'
UNION ALL
SELECT 17,0,215114,0,6,15,0,8,0,0,0,0,'','Kingslayers native pickup - owned final-stage Rogue'
UNION ALL
SELECT 17,0,215114,0,6,22,0,1620,0,0,0,0,'','Kingslayers native pickup - owned final-stage Rogue'
UNION ALL
SELECT 17,0,215114,0,6,28,0,42627,0,0,0,0,'','Kingslayers native pickup - owned final-stage Rogue'
UNION ALL
SELECT 17,0,215114,0,6,36,0,0,0,0,0,0,'','Kingslayers native pickup - owned final-stage Rogue'
UNION ALL
SELECT 17,0,215114,0,6,44,0,1123,8,0,0,0,'','Kingslayers native pickup - owned final-stage Rogue'
UNION ALL
SELECT 17,0,215114,0,7,2,0,128870,1,0,0,0,'','Kingslayers native pickup - owned final-stage Rogue'
UNION ALL
SELECT 17,0,215114,0,7,8,0,42627,0,0,1,0,'','Kingslayers native pickup - owned final-stage Rogue'
UNION ALL
SELECT 17,0,215114,0,7,15,0,8,0,0,0,0,'','Kingslayers native pickup - owned final-stage Rogue'
UNION ALL
SELECT 17,0,215114,0,7,22,0,1620,0,0,0,0,'','Kingslayers native pickup - owned final-stage Rogue'
UNION ALL
SELECT 17,0,215114,0,7,28,0,42627,0,0,0,0,'','Kingslayers native pickup - owned final-stage Rogue'
UNION ALL
SELECT 17,0,215114,0,7,36,0,0,0,0,0,0,'','Kingslayers native pickup - owned final-stage Rogue'
UNION ALL
SELECT 17,0,215114,0,7,44,0,1123,9,0,0,0,'','Kingslayers native pickup - owned final-stage Rogue'
) AS native_rows WHERE @kingslayers_safe;
COMMIT;
SET SESSION group_concat_max_len=@kingslayers_previous_concat;
