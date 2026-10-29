-- Native package1631 ends the Feral presentation; it has no custom equip trigger.
-- Use the core's real completion callback and native existing-item equip spell.
DROP TEMPORARY TABLE IF EXISTS feral_scene_expected;
CREATE TEMPORARY TABLE feral_scene_expected LIKE conditions;
INSERT INTO feral_scene_expected VALUES
(17,0,214715,0,0,22,0,1612,0,0,1,0,'','Feral equip: preserve native use outside the scenario map');
INSERT INTO feral_scene_expected
(SourceTypeOrReferenceId,SourceGroup,SourceEntry,SourceId,ElseGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2,ConditionValue3,NegativeCondition,ErrorTextId,ScriptName,Comment)
SELECT 17,0,214715,0,variant.grp,IF(rule.kind=0,variant.quest,rule.kind),0,rule.value1,
 IF(rule.kind=44,variant.stage,IF(rule.kind=2,1,0)),0,rule.negative,0,'',
 'Feral equip: real scene completion and owned native final-stage item'
FROM (SELECT 1 grp,6 stage,9 quest UNION ALL SELECT 2,6,28 UNION ALL SELECT 3,7,9 UNION ALL SELECT 4,7,28) variant
CROSS JOIN (SELECT 2 kind,128860 value1,0 negative UNION ALL SELECT 15,1024,0
 UNION ALL SELECT 22,1612,0 UNION ALL SELECT 44,1108,0 UNION ALL SELECT 0,42430,0
 UNION ALL SELECT 8,42430,1 UNION ALL SELECT 36,0,0 UNION ALL SELECT 40,1282,0) rule;
START TRANSACTION;
SET @feral_scene_safe :=
 (SELECT COUNT(*) FROM spell_scene WHERE MiscValue=1282 AND SceneScriptPackageID=1631 AND PlaybackFlags=21 AND CustomDuration=0 AND ScriptName IS NULL AND comment='')=1
 AND ((SELECT COUNT(*) FROM spell_scene_event WHERE MiscValue=1282)=0 OR
 ((SELECT COUNT(*) FROM spell_scene_event WHERE MiscValue=1282)=1 AND
 (SELECT COUNT(*) FROM spell_scene_event WHERE MiscValue=1282 AND Event='complete' AND trigerSpell=214715 AND MonsterCredit=0 AND comment='Feral native scene completion equips owned Fangs')=1))
 AND ((SELECT COUNT(*) FROM conditions WHERE SourceTypeOrReferenceId=17 AND SourceEntry=214715)=0 OR
 ((SELECT COUNT(*) FROM conditions WHERE SourceTypeOrReferenceId=17 AND SourceEntry=214715)=33 AND NOT EXISTS
 (SELECT 1 FROM conditions c WHERE c.SourceTypeOrReferenceId=17 AND c.SourceEntry=214715 AND NOT EXISTS
 (SELECT 1 FROM feral_scene_expected e WHERE c.SourceGroup=e.SourceGroup AND c.SourceId=e.SourceId AND c.ElseGroup=e.ElseGroup
 AND c.ConditionTypeOrReference=e.ConditionTypeOrReference AND c.ConditionTarget=e.ConditionTarget
 AND c.ConditionValue1=e.ConditionValue1 AND c.ConditionValue2=e.ConditionValue2 AND c.ConditionValue3=e.ConditionValue3
 AND c.NegativeCondition=e.NegativeCondition AND c.ErrorTextId=e.ErrorTextId AND c.ScriptName=e.ScriptName AND c.Comment=e.Comment)))
 AND NOT EXISTS (SELECT 1 FROM feral_scene_expected e WHERE NOT EXISTS
 (SELECT 1 FROM conditions c WHERE c.SourceTypeOrReferenceId=e.SourceTypeOrReferenceId AND c.SourceEntry=e.SourceEntry
 AND c.SourceGroup=e.SourceGroup AND c.SourceId=e.SourceId AND c.ElseGroup=e.ElseGroup
 AND c.ConditionTypeOrReference=e.ConditionTypeOrReference AND c.ConditionTarget=e.ConditionTarget
 AND c.ConditionValue1=e.ConditionValue1 AND c.ConditionValue2=e.ConditionValue2 AND c.ConditionValue3=e.ConditionValue3
 AND c.NegativeCondition=e.NegativeCondition AND c.ErrorTextId=e.ErrorTextId AND c.ScriptName=e.ScriptName AND c.Comment=e.Comment)));
SET @feral_scene_check := IF(COALESCE(@feral_scene_safe,0),'DO 0','SIGNAL SQLSTATE ''45000'' SET MESSAGE_TEXT=''Feral native scene/equip conditions differ; review before applying''');
PREPARE feral_scene_check FROM @feral_scene_check;EXECUTE feral_scene_check;DEALLOCATE PREPARE feral_scene_check;
DELETE FROM spell_scene_event WHERE @feral_scene_safe AND MiscValue=1282;
INSERT INTO spell_scene_event SELECT 1282,'complete',214715,0,'Feral native scene completion equips owned Fangs' WHERE @feral_scene_safe;
DELETE FROM conditions WHERE @feral_scene_safe AND SourceTypeOrReferenceId=17 AND SourceEntry=214715;
INSERT INTO conditions SELECT * FROM feral_scene_expected WHERE @feral_scene_safe;
COMMIT;
DROP TEMPORARY TABLE feral_scene_expected;
