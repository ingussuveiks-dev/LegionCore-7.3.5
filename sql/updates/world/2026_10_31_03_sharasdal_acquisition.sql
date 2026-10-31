-- Keep outside live updater input until matching acquisition loaders are linked.
-- Native storage owns128911; native249 owns equip; native61/134 own progression.
DROP TEMPORARY TABLE IF EXISTS sharasdal_return_expected;
CREATE TEMPORARY TABLE sharasdal_return_expected LIKE conditions;
INSERT INTO sharasdal_return_expected(SourceTypeOrReferenceId,SourceGroup,SourceEntry,ElseGroup,
 ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2,ConditionValue3,NegativeCondition,Comment)
SELECT 18,105819,210252,g.n,c.kind,0,c.v1,c.v2,c.v3,c.neg,'Sharasdal: real acquired artifact and active original return only'
FROM (SELECT 0 n UNION ALL SELECT 1) g
CROSS JOIN (
 SELECT 32 kind,16 v1,0 v2,0 v3,0 neg
 UNION ALL SELECT 15,64,0,0,0
 UNION ALL SELECT 22,1600,0,0,0
 UNION ALL SELECT 36,0,0,0,0
 UNION ALL SELECT 44,1066,6,0,0
 UNION ALL SELECT 41,40341,105787,1,0
 UNION ALL SELECT 2,128911,1,1,0
 UNION ALL SELECT 1,209684,0,0,1
) c
UNION ALL SELECT 18,105819,210252,0,9,0,40341,0,0,0,'Sharasdal: incomplete original return'
UNION ALL SELECT 18,105819,210252,1,28,0,40341,0,0,0,'Sharasdal: complete unturned original return';

SET @sharasdal_shape :=
 (SELECT COUNT(*) FROM gameobject_template WHERE entry=249342 AND type=3 AND displayId=9806 AND size=2
  AND flags=2113536 AND data0=1691 AND data1 IN(0,249342) AND data2=1 AND AIName='' AND ScriptName='go_art_sharasdal')=1
 AND (SELECT COUNT(*) FROM gameobject WHERE id=249342)=1
 AND EXISTS(SELECT 1 FROM gameobject WHERE guid=134299 AND id=249342 AND map=1600 AND PhaseId='7002'
  AND ABS(position_x-50.5781)<0.001 AND ABS(position_y-802.392)<0.001 AND ABS(position_z-805.731)<0.001 AND orientation=0)
 AND (SELECT COUNT(*) FROM gameobject_loot_template WHERE entry=249342)=1
 AND EXISTS(SELECT 1 FROM gameobject_loot_template WHERE entry=249342 AND item=128911 AND Currency=0 AND Reference=0 AND Chance=100
  AND QuestRequired=0 AND LootMode=1 AND GroupId=0 AND MinCount=1 AND MaxCount=1)
 AND (SELECT COUNT(*) FROM creature WHERE id=105819)=1
 AND EXISTS(SELECT 1 FROM creature WHERE guid=362097 AND id=105819 AND map=1600 AND PhaseId='7003')
 AND EXISTS(SELECT 1 FROM creature_template WHERE entry=105819 AND npcflag=16777216 AND AIName='' AND ScriptName='')
 AND (SELECT COUNT(*) FROM npc_spellclick_spells WHERE npc_entry=105819)=1
 AND EXISTS(SELECT 1 FROM npc_spellclick_spells WHERE npc_entry=105819 AND spell_id=210252 AND cast_flags=1 AND user_type=0 AND add_npc_flag=1)
 AND EXISTS(SELECT 1 FROM spell_target_position WHERE id=210252 AND target_map=1469
  AND ABS(target_position_x-836.16)<0.001 AND ABS(target_position_y-1035.4)<0.001
  AND ABS(target_position_z-48.61)<0.001 AND ABS(target_orientation-1.7935)<0.0001)
 AND EXISTS(SELECT 1 FROM spell_scene WHERE MiscValue=1255 AND SceneScriptPackageID=1609 AND PlaybackFlags=17 AND CustomDuration=0 AND (ScriptName IS NULL OR ScriptName=''))
 AND NOT EXISTS(SELECT 1 FROM conditions WHERE SourceTypeOrReferenceId IN(13,17) AND SourceEntry IN(209683,209684,209685))
 AND EXISTS(SELECT 1 FROM quest_objectives WHERE ID=283384 AND QuestID=40341 AND Type=0 AND StorageIndex=1 AND ObjectID=105787 AND Amount=1 AND Flags=2)
 AND EXISTS(SELECT 1 FROM quest_objectives WHERE ID=283385 AND QuestID=40341 AND Type=0 AND StorageIndex=2 AND ObjectID=105806 AND Amount=1 AND Flags=2);

SET @sharasdal_original :=
 EXISTS(SELECT 1 FROM gameobject_template WHERE entry=249342 AND data1=0)
 AND NOT EXISTS(SELECT 1 FROM spell_target_position WHERE id=209683)
 AND NOT EXISTS(SELECT 1 FROM spell_script_names WHERE spell_id IN(209683,209684,209685))
 AND NOT EXISTS(SELECT 1 FROM conditions WHERE SourceTypeOrReferenceId=18 AND SourceGroup=105819);
SET @sharasdal_applied :=
 EXISTS(SELECT 1 FROM gameobject_template WHERE entry=249342 AND data1=249342)
 AND EXISTS(SELECT 1 FROM spell_target_position WHERE id=209683 AND target_map=1600
  AND ABS(target_position_x-50.5781)<0.001 AND ABS(target_position_y-802.392)<0.001
  AND ABS(target_position_z-805.731)<0.001 AND target_orientation=0)
 AND (SELECT COUNT(*) FROM spell_script_names WHERE spell_id IN(209683,209684,209685))=4
 AND (SELECT COUNT(*) FROM spell_script_names WHERE spell_id IN(209683,209684,209685) AND
  (ScriptName='spell_sharasdal_acquisition' OR (spell_id=209684 AND ScriptName='aura_sharasdal_acquisition')))=4
 AND (SELECT COUNT(*) FROM conditions WHERE SourceTypeOrReferenceId=18 AND SourceGroup=105819)=18
 -- Accept the exact pre-commit inventory-only profile too, so its two bank
 -- bits can be upgraded without rewriting any other source18 condition.
 AND (SELECT COUNT(DISTINCT ConditionValue3) FROM conditions WHERE SourceTypeOrReferenceId=18
  AND SourceGroup=105819 AND ConditionTypeOrReference=2)=1
 AND NOT EXISTS(SELECT 1 FROM conditions c LEFT JOIN sharasdal_return_expected e
  ON c.SourceTypeOrReferenceId=e.SourceTypeOrReferenceId AND c.SourceGroup=e.SourceGroup AND c.SourceEntry=e.SourceEntry
  AND c.SourceId=e.SourceId AND c.ElseGroup=e.ElseGroup AND c.ConditionTypeOrReference=e.ConditionTypeOrReference
  AND c.ConditionTarget=e.ConditionTarget AND c.ConditionValue1=e.ConditionValue1 AND c.ConditionValue2=e.ConditionValue2
  AND (c.ConditionValue3=e.ConditionValue3 OR (c.ConditionTypeOrReference=2 AND c.ConditionValue3=0 AND e.ConditionValue3=1))
  AND c.NegativeCondition=e.NegativeCondition AND c.ErrorTextId=e.ErrorTextId AND c.ScriptName=e.ScriptName
  WHERE c.SourceTypeOrReferenceId=18 AND c.SourceGroup=105819 AND e.SourceEntry IS NULL);
SET @sharasdal_apply := @sharasdal_shape AND (@sharasdal_original OR @sharasdal_applied);
SET @sharasdal_guard := IF(@sharasdal_apply,'DO 0',
 'SIGNAL SQLSTATE ''45000'' SET MESSAGE_TEXT=''Sharasdal acquisition context differs; review before updating''');
PREPARE sharasdal_guard_stmt FROM @sharasdal_guard;
EXECUTE sharasdal_guard_stmt;
DEALLOCATE PREPARE sharasdal_guard_stmt;

START TRANSACTION;
UPDATE gameobject_template SET data1=249342 WHERE entry=249342 AND @sharasdal_apply AND @sharasdal_original;
-- All25 actual7.3.5 VMAP samples at the original object position supportZ805.730774.
INSERT INTO spell_target_position(id,target_map,target_position_x,target_position_y,target_position_z,target_orientation)
 SELECT 209683,1600,50.5781,802.392,805.731,0 WHERE @sharasdal_apply AND @sharasdal_original;
INSERT INTO spell_script_names(spell_id,ScriptName)
 SELECT 209683,'spell_sharasdal_acquisition' WHERE @sharasdal_apply AND @sharasdal_original
 UNION ALL SELECT 209684,'spell_sharasdal_acquisition' WHERE @sharasdal_apply AND @sharasdal_original
 UNION ALL SELECT 209685,'spell_sharasdal_acquisition' WHERE @sharasdal_apply AND @sharasdal_original
 UNION ALL SELECT 209684,'aura_sharasdal_acquisition' WHERE @sharasdal_apply AND @sharasdal_original;
INSERT INTO conditions SELECT * FROM sharasdal_return_expected WHERE @sharasdal_apply AND @sharasdal_original;
-- Banking a genuinely acquired weapon does not revoke its native quest/root
-- receipts. Bank-only ownership before acquisition still cannot pass them.
UPDATE conditions SET ConditionValue3=1 WHERE SourceTypeOrReferenceId=18 AND SourceGroup=105819 AND SourceEntry=210252
 AND ConditionTypeOrReference=2 AND ConditionValue1=128911 AND ConditionValue2=1 AND ConditionValue3=0 AND @sharasdal_apply;
COMMIT;
DROP TEMPORARY TABLE sharasdal_return_expected;
