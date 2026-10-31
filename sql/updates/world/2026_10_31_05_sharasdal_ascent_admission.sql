-- Native1247/1604/208580/-208580/209992 alone owns camera, transfer and50336.
-- Check the original player again when the delayed native exit is cast.
DROP TEMPORARY TABLE IF EXISTS sharasdal_ascent_expected;
CREATE TEMPORARY TABLE sharasdal_ascent_expected LIKE conditions;
INSERT INTO sharasdal_ascent_expected(SourceTypeOrReferenceId,SourceGroup,SourceEntry,ElseGroup,
 ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2,ConditionValue3,NegativeCondition,Comment)
SELECT p.source,p.grp,p.spell,g.n,c.kind,0,c.v1,c.v2,0,0,'Sharasdal: active native elevator participant only'
FROM (SELECT 17 source,0 grp,208580 spell UNION ALL SELECT 17,0,209992 UNION ALL SELECT 18,38821,208580) p
CROSS JOIN (SELECT 0 n UNION ALL SELECT 1) g
CROSS JOIN (
 SELECT 32 kind,16 v1,0 v2
 UNION ALL SELECT 15,64,0
 UNION ALL SELECT 22,1600,0
 UNION ALL SELECT 36,0,0
 UNION ALL SELECT 44,1066,3
) c
UNION ALL
SELECT p.source,p.grp,p.spell,g.n,IF(g.n=0,9,28),0,40341,0,0,0,
 'Sharasdal: active native elevator participant only'
FROM (SELECT 17 source,0 grp,208580 spell UNION ALL SELECT 17,0,209992 UNION ALL SELECT 18,38821,208580) p
CROSS JOIN (SELECT 0 n UNION ALL SELECT 1) g;

SET @sharasdal_ascent_shape :=
 (SELECT COUNT(*) FROM npc_spellclick_spells WHERE npc_entry=38821 AND spell_id=208580)=1
 AND EXISTS(SELECT 1 FROM npc_spellclick_spells WHERE npc_entry=38821 AND spell_id=208580
  AND cast_flags=3 AND user_type=0 AND add_npc_flag=1)
 AND (SELECT COUNT(*) FROM creature WHERE id=38821 AND map=1600)=1
 AND EXISTS(SELECT 1 FROM creature WHERE guid=362075 AND id=38821 AND map=1600 AND PhaseID='7001'
  AND ABS(position_x+215.63)<0.001 AND ABS(position_y-808.604)<0.001 AND ABS(position_z-262.383)<0.001)
 AND (SELECT COUNT(*) FROM spell_scene WHERE MiscValue=1247)=1
 AND EXISTS(SELECT 1 FROM spell_scene WHERE MiscValue=1247 AND PlaybackFlags=20 AND SceneScriptPackageID=1604
  AND CustomDuration=0 AND (ScriptName IS NULL OR ScriptName=''))
 AND NOT EXISTS(SELECT 1 FROM spell_scene_event WHERE MiscValue=1247)
 AND (SELECT COUNT(*) FROM spell_target_position WHERE id=209992)=1
 AND EXISTS(SELECT 1 FROM spell_target_position WHERE id=209992 AND target_map=1600
  AND ABS(target_position_x+187.08)<0.001 AND ABS(target_position_y-802.57)<0.001
  AND ABS(target_position_z-796.5)<0.001 AND target_orientation=0)
 AND (SELECT COUNT(*) FROM spell_linked_spell WHERE ABS(spell_trigger) IN(208580,209992)
  OR ABS(spell_effect) IN(208580,209992))=1
 AND EXISTS(SELECT 1 FROM spell_linked_spell WHERE spell_trigger=-208580 AND spell_effect=209992 AND type=0
  AND caster=0 AND target=0 AND hastalent=0 AND hastalent2=0 AND chance=0 AND cooldown=0
  AND hastype=0 AND hitmask=0 AND removeMask=0 AND hastype2=0 AND actiontype=0 AND targetCount=-1
  AND targetCountType=0 AND `group`=0 AND duration=0 AND param=0 AND effectMask=0 AND hasparam=0
  AND hasparam2=0 AND (randList IS NULL OR randList=''))
 AND NOT EXISTS(SELECT 1 FROM spell_script_names WHERE spell_id IN(208580,209992))
 AND NOT EXISTS(SELECT 1 FROM conditions WHERE SourceTypeOrReferenceId=13 AND SourceEntry IN(208580,209992));

SET @sharasdal_ascent_original := NOT EXISTS(SELECT 1 FROM conditions
 WHERE (SourceTypeOrReferenceId=17 AND SourceEntry IN(208580,209992))
 OR (SourceTypeOrReferenceId=18 AND SourceGroup=38821 AND SourceEntry=208580));
SET @sharasdal_ascent_applied :=
 (SELECT COUNT(*) FROM conditions WHERE (SourceTypeOrReferenceId=17 AND SourceEntry IN(208580,209992))
  OR (SourceTypeOrReferenceId=18 AND SourceGroup=38821 AND SourceEntry=208580))=36
 AND NOT EXISTS(SELECT 1 FROM conditions c LEFT JOIN sharasdal_ascent_expected e
  ON c.SourceTypeOrReferenceId=e.SourceTypeOrReferenceId AND c.SourceGroup=e.SourceGroup AND c.SourceEntry=e.SourceEntry
  AND c.SourceId=e.SourceId AND c.ElseGroup=e.ElseGroup AND c.ConditionTypeOrReference=e.ConditionTypeOrReference
  AND c.ConditionTarget=e.ConditionTarget AND c.ConditionValue1=e.ConditionValue1 AND c.ConditionValue2=e.ConditionValue2
  AND c.ConditionValue3=e.ConditionValue3 AND c.NegativeCondition=e.NegativeCondition
  AND c.ErrorTextId=e.ErrorTextId AND c.ScriptName=e.ScriptName
  WHERE ((c.SourceTypeOrReferenceId=17 AND c.SourceEntry IN(208580,209992))
   OR (c.SourceTypeOrReferenceId=18 AND c.SourceGroup=38821 AND c.SourceEntry=208580)) AND e.SourceEntry IS NULL);
SET @sharasdal_ascent_apply := @sharasdal_ascent_shape AND (@sharasdal_ascent_original OR @sharasdal_ascent_applied);
SET @sharasdal_ascent_guard := IF(@sharasdal_ascent_apply,'DO 0',
 'SIGNAL SQLSTATE ''45000'' SET MESSAGE_TEXT=''Sharasdal ascent context differs; review before updating''');
PREPARE sharasdal_ascent_guard_stmt FROM @sharasdal_ascent_guard;
EXECUTE sharasdal_ascent_guard_stmt;
DEALLOCATE PREPARE sharasdal_ascent_guard_stmt;

START TRANSACTION;
INSERT INTO conditions SELECT * FROM sharasdal_ascent_expected
 WHERE @sharasdal_ascent_apply AND @sharasdal_ascent_original;
COMMIT;
DROP TEMPORARY TABLE sharasdal_ascent_expected;
