-- Native1192/package1557 equips only a visual clone.
-- Its terminal completion requests202438 for an already owned128937.
-- Physical pickup, original grants and return are separate unresolved work.
SET @sheilun_scene_concat=@@SESSION.group_concat_max_len;
SET SESSION group_concat_max_len=GREATEST(@sheilun_scene_concat,1048576);
START TRANSACTION;
SET @sheilun_scene_spell_scene=(SELECT SHA2(COALESCE(GROUP_CONCAT(CONCAT_WS('|',COALESCE(CAST(`SceneScriptPackageID` AS CHAR CHARACTER SET utf8mb4),'NULL'),COALESCE(CAST(`MiscValue` AS CHAR CHARACTER SET utf8mb4),'NULL'),COALESCE(CAST(`PlaybackFlags` AS CHAR CHARACTER SET utf8mb4),'NULL'),COALESCE(CAST(`CustomDuration` AS CHAR CHARACTER SET utf8mb4),'NULL'),COALESCE(CAST(`ScriptName` AS CHAR CHARACTER SET utf8mb4),'NULL'),COALESCE(CAST(`comment` AS CHAR CHARACTER SET utf8mb4),'NULL')) ORDER BY `SceneScriptPackageID`,`MiscValue`,`PlaybackFlags`,`CustomDuration`,`ScriptName`,`comment` SEPARATOR '\n'),''),256) FROM spell_scene WHERE MiscValue=1192);
SET @sheilun_scene_spell_scene_event=(SELECT SHA2(COALESCE(GROUP_CONCAT(CONCAT_WS('|',COALESCE(CAST(`MiscValue` AS CHAR CHARACTER SET utf8mb4),'NULL'),COALESCE(CAST(`Event` AS CHAR CHARACTER SET utf8mb4),'NULL'),COALESCE(CAST(`trigerSpell` AS CHAR CHARACTER SET utf8mb4),'NULL'),COALESCE(CAST(`MonsterCredit` AS CHAR CHARACTER SET utf8mb4),'NULL'),COALESCE(CAST(`comment` AS CHAR CHARACTER SET utf8mb4),'NULL')) ORDER BY `MiscValue`,`Event`,`trigerSpell`,`MonsterCredit`,`comment` SEPARATOR '\n'),''),256) FROM spell_scene_event WHERE MiscValue=1192);
SET @sheilun_scene_conditions=(SELECT SHA2(COALESCE(GROUP_CONCAT(CONCAT_WS('|',COALESCE(CAST(`SourceTypeOrReferenceId` AS CHAR CHARACTER SET utf8mb4),'NULL'),COALESCE(CAST(`SourceGroup` AS CHAR CHARACTER SET utf8mb4),'NULL'),COALESCE(CAST(`SourceEntry` AS CHAR CHARACTER SET utf8mb4),'NULL'),COALESCE(CAST(`SourceId` AS CHAR CHARACTER SET utf8mb4),'NULL'),COALESCE(CAST(`ElseGroup` AS CHAR CHARACTER SET utf8mb4),'NULL'),COALESCE(CAST(`ConditionTypeOrReference` AS CHAR CHARACTER SET utf8mb4),'NULL'),COALESCE(CAST(`ConditionTarget` AS CHAR CHARACTER SET utf8mb4),'NULL'),COALESCE(CAST(`ConditionValue1` AS CHAR CHARACTER SET utf8mb4),'NULL'),COALESCE(CAST(`ConditionValue2` AS CHAR CHARACTER SET utf8mb4),'NULL'),COALESCE(CAST(`ConditionValue3` AS CHAR CHARACTER SET utf8mb4),'NULL'),COALESCE(CAST(`NegativeCondition` AS CHAR CHARACTER SET utf8mb4),'NULL'),COALESCE(CAST(`ErrorTextId` AS CHAR CHARACTER SET utf8mb4),'NULL'),COALESCE(CAST(`ScriptName` AS CHAR CHARACTER SET utf8mb4),'NULL'),COALESCE(CAST(`Comment` AS CHAR CHARACTER SET utf8mb4),'NULL')) ORDER BY `SourceTypeOrReferenceId`,`SourceGroup`,`SourceEntry`,`SourceId`,`ElseGroup`,`ConditionTypeOrReference`,`ConditionTarget`,`ConditionValue1`,`ConditionValue2`,`ConditionValue3`,`NegativeCondition`,`ErrorTextId`,`ScriptName`,`Comment` SEPARATOR '\n'),''),256) FROM conditions WHERE SourceTypeOrReferenceId=17 AND SourceEntry=202438);
SET @sheilun_scene_old=@sheilun_scene_spell_scene='528472d99ebc0069870aa64d57feb522619ef5439bbc8a24b05ae62957399b13' AND @sheilun_scene_spell_scene_event='e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855' AND @sheilun_scene_conditions='e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855';
SET @sheilun_scene_applied=@sheilun_scene_spell_scene='d770cf51ee3b04763dc10b6fc37816f3d7d72e3529518b8a6f66bc8e8cc41192' AND @sheilun_scene_spell_scene_event='d9be84f00c0a32f9b2a2a3d924ff6d149000e0799f03e1fb78134e1175d99fbb' AND @sheilun_scene_conditions='0acd4a144f6f281e994afaa487fe8311c115ca4968362fb312a2959beee087ec';
SET @sheilun_scene_safe=COALESCE(@sheilun_scene_old OR @sheilun_scene_applied,0);
SET @sheilun_scene_guard=IF(@sheilun_scene_safe,'DO 0','SIGNAL SQLSTATE ''45000'' SET MESSAGE_TEXT=''Sheilun scene/equip/admission differs; review before applying''');
PREPARE sheilun_scene_guard FROM @sheilun_scene_guard;EXECUTE sheilun_scene_guard;DEALLOCATE PREPARE sheilun_scene_guard;
UPDATE spell_scene SET ScriptName='scene_monk_artifact_equipment' WHERE MiscValue=1192 AND @sheilun_scene_safe AND @sheilun_scene_old;
INSERT INTO spell_scene_event(`MiscValue`,`Event`,`trigerSpell`,`MonsterCredit`,`comment`)
SELECT 1192,'complete',202438,0,'Sheilun scene completion equips owned artifact' WHERE @sheilun_scene_safe AND @sheilun_scene_old;
INSERT INTO conditions(`SourceTypeOrReferenceId`,`SourceGroup`,`SourceEntry`,`SourceId`,`ElseGroup`,`ConditionTypeOrReference`,`ConditionTarget`,`ConditionValue1`,`ConditionValue2`,`ConditionValue3`,`NegativeCondition`,`ErrorTextId`,`ScriptName`,`Comment`)
SELECT 17,0,202438,0,0,22,0,1541,0,0,1,0,'','Sheilun equip: preserve native use outside acquisition map' WHERE @sheilun_scene_safe AND @sheilun_scene_old
UNION ALL SELECT 17,0,202438,0,1,2,0,128937,1,0,0,0,'','Sheilun equip: completed native scene and owned artifact' WHERE @sheilun_scene_safe AND @sheilun_scene_old
UNION ALL SELECT 17,0,202438,0,1,15,0,512,0,0,0,0,'','Sheilun equip: completed native scene and owned artifact' WHERE @sheilun_scene_safe AND @sheilun_scene_old
UNION ALL SELECT 17,0,202438,0,1,22,0,1541,0,0,0,0,'','Sheilun equip: completed native scene and owned artifact' WHERE @sheilun_scene_safe AND @sheilun_scene_old
UNION ALL SELECT 17,0,202438,0,1,44,0,1007,4,0,0,0,'','Sheilun equip: completed native scene and owned artifact' WHERE @sheilun_scene_safe AND @sheilun_scene_old
UNION ALL SELECT 17,0,202438,0,1,9,0,41003,0,0,0,0,'','Sheilun equip: completed native scene and owned artifact' WHERE @sheilun_scene_safe AND @sheilun_scene_old
UNION ALL SELECT 17,0,202438,0,1,8,0,41003,0,0,1,0,'','Sheilun equip: completed native scene and owned artifact' WHERE @sheilun_scene_safe AND @sheilun_scene_old
UNION ALL SELECT 17,0,202438,0,1,36,0,0,0,0,0,0,'','Sheilun equip: completed native scene and owned artifact' WHERE @sheilun_scene_safe AND @sheilun_scene_old
UNION ALL SELECT 17,0,202438,0,1,40,0,1192,0,0,0,0,'','Sheilun equip: completed native scene and owned artifact' WHERE @sheilun_scene_safe AND @sheilun_scene_old
UNION ALL SELECT 17,0,202438,0,2,2,0,128937,1,0,0,0,'','Sheilun equip: completed native scene and owned artifact' WHERE @sheilun_scene_safe AND @sheilun_scene_old
UNION ALL SELECT 17,0,202438,0,2,15,0,512,0,0,0,0,'','Sheilun equip: completed native scene and owned artifact' WHERE @sheilun_scene_safe AND @sheilun_scene_old
UNION ALL SELECT 17,0,202438,0,2,22,0,1541,0,0,0,0,'','Sheilun equip: completed native scene and owned artifact' WHERE @sheilun_scene_safe AND @sheilun_scene_old
UNION ALL SELECT 17,0,202438,0,2,44,0,1007,4,0,0,0,'','Sheilun equip: completed native scene and owned artifact' WHERE @sheilun_scene_safe AND @sheilun_scene_old
UNION ALL SELECT 17,0,202438,0,2,28,0,41003,0,0,0,0,'','Sheilun equip: completed native scene and owned artifact' WHERE @sheilun_scene_safe AND @sheilun_scene_old
UNION ALL SELECT 17,0,202438,0,2,8,0,41003,0,0,1,0,'','Sheilun equip: completed native scene and owned artifact' WHERE @sheilun_scene_safe AND @sheilun_scene_old
UNION ALL SELECT 17,0,202438,0,2,36,0,0,0,0,0,0,'','Sheilun equip: completed native scene and owned artifact' WHERE @sheilun_scene_safe AND @sheilun_scene_old
UNION ALL SELECT 17,0,202438,0,2,40,0,1192,0,0,0,0,'','Sheilun equip: completed native scene and owned artifact' WHERE @sheilun_scene_safe AND @sheilun_scene_old
UNION ALL SELECT 17,0,202438,0,3,2,0,128937,1,0,0,0,'','Sheilun equip: completed native scene and owned artifact' WHERE @sheilun_scene_safe AND @sheilun_scene_old
UNION ALL SELECT 17,0,202438,0,3,15,0,512,0,0,0,0,'','Sheilun equip: completed native scene and owned artifact' WHERE @sheilun_scene_safe AND @sheilun_scene_old
UNION ALL SELECT 17,0,202438,0,3,22,0,1541,0,0,0,0,'','Sheilun equip: completed native scene and owned artifact' WHERE @sheilun_scene_safe AND @sheilun_scene_old
UNION ALL SELECT 17,0,202438,0,3,44,0,1007,5,0,0,0,'','Sheilun equip: completed native scene and owned artifact' WHERE @sheilun_scene_safe AND @sheilun_scene_old
UNION ALL SELECT 17,0,202438,0,3,9,0,41003,0,0,0,0,'','Sheilun equip: completed native scene and owned artifact' WHERE @sheilun_scene_safe AND @sheilun_scene_old
UNION ALL SELECT 17,0,202438,0,3,8,0,41003,0,0,1,0,'','Sheilun equip: completed native scene and owned artifact' WHERE @sheilun_scene_safe AND @sheilun_scene_old
UNION ALL SELECT 17,0,202438,0,3,36,0,0,0,0,0,0,'','Sheilun equip: completed native scene and owned artifact' WHERE @sheilun_scene_safe AND @sheilun_scene_old
UNION ALL SELECT 17,0,202438,0,3,40,0,1192,0,0,0,0,'','Sheilun equip: completed native scene and owned artifact' WHERE @sheilun_scene_safe AND @sheilun_scene_old
UNION ALL SELECT 17,0,202438,0,4,2,0,128937,1,0,0,0,'','Sheilun equip: completed native scene and owned artifact' WHERE @sheilun_scene_safe AND @sheilun_scene_old
UNION ALL SELECT 17,0,202438,0,4,15,0,512,0,0,0,0,'','Sheilun equip: completed native scene and owned artifact' WHERE @sheilun_scene_safe AND @sheilun_scene_old
UNION ALL SELECT 17,0,202438,0,4,22,0,1541,0,0,0,0,'','Sheilun equip: completed native scene and owned artifact' WHERE @sheilun_scene_safe AND @sheilun_scene_old
UNION ALL SELECT 17,0,202438,0,4,44,0,1007,5,0,0,0,'','Sheilun equip: completed native scene and owned artifact' WHERE @sheilun_scene_safe AND @sheilun_scene_old
UNION ALL SELECT 17,0,202438,0,4,28,0,41003,0,0,0,0,'','Sheilun equip: completed native scene and owned artifact' WHERE @sheilun_scene_safe AND @sheilun_scene_old
UNION ALL SELECT 17,0,202438,0,4,8,0,41003,0,0,1,0,'','Sheilun equip: completed native scene and owned artifact' WHERE @sheilun_scene_safe AND @sheilun_scene_old
UNION ALL SELECT 17,0,202438,0,4,36,0,0,0,0,0,0,'','Sheilun equip: completed native scene and owned artifact' WHERE @sheilun_scene_safe AND @sheilun_scene_old
UNION ALL SELECT 17,0,202438,0,4,40,0,1192,0,0,0,0,'','Sheilun equip: completed native scene and owned artifact' WHERE @sheilun_scene_safe AND @sheilun_scene_old;
COMMIT;
SET SESSION group_concat_max_len=@sheilun_scene_concat;
