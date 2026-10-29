-- CODE-COUPLED: keep pending until npc_stromkar_valkyr/condition_stromkar_return
-- and Thoradin's real final TEXT_OVER handoff are in the promoted executable.
SET @stromkar_return_concat := @@SESSION.group_concat_max_len;
SET SESSION group_concat_max_len=GREATEST(@@SESSION.group_concat_max_len,1048576);
START TRANSACTION;
SET @stromkar_return_ghost := (SELECT SHA2(GROUP_CONCAT(CONCAT_WS('|',COALESCE(CAST(`entryorguid` AS CHAR),'NULL'),COALESCE(CAST(`source_type` AS CHAR),'NULL'),COALESCE(CAST(`id` AS CHAR),'NULL'),COALESCE(CAST(`link` AS CHAR),'NULL'),COALESCE(CAST(`Difficulties` AS CHAR),'NULL'),COALESCE(CAST(`event_type` AS CHAR),'NULL'),COALESCE(CAST(`event_phase_mask` AS CHAR),'NULL'),COALESCE(CAST(`event_chance` AS CHAR),'NULL'),COALESCE(CAST(`event_flags` AS CHAR),'NULL'),COALESCE(CAST(`event_param1` AS CHAR),'NULL'),COALESCE(CAST(`event_param2` AS CHAR),'NULL'),COALESCE(CAST(`event_param3` AS CHAR),'NULL'),COALESCE(CAST(`event_param4` AS CHAR),'NULL'),COALESCE(CAST(`event_param5` AS CHAR),'NULL'),COALESCE(CAST(`action_type` AS CHAR),'NULL'),COALESCE(CAST(`action_param1` AS CHAR),'NULL'),COALESCE(CAST(`action_param2` AS CHAR),'NULL'),COALESCE(CAST(`action_param3` AS CHAR),'NULL'),COALESCE(CAST(`action_param4` AS CHAR),'NULL'),COALESCE(CAST(`action_param5` AS CHAR),'NULL'),COALESCE(CAST(`action_param6` AS CHAR),'NULL'),COALESCE(CAST(`target_type` AS CHAR),'NULL'),COALESCE(CAST(`target_param1` AS CHAR),'NULL'),COALESCE(CAST(`target_param2` AS CHAR),'NULL'),COALESCE(CAST(`target_param3` AS CHAR),'NULL'),COALESCE(CAST(`target_param4` AS CHAR),'NULL'),COALESCE(CAST(`target_x` AS CHAR),'NULL'),COALESCE(CAST(`target_y` AS CHAR),'NULL'),COALESCE(CAST(`target_z` AS CHAR),'NULL'),COALESCE(CAST(`target_o` AS CHAR),'NULL'),COALESCE(CAST(`comment` AS CHAR),'NULL')) ORDER BY `entryorguid`,`source_type`,`id`,`link`,`Difficulties`,`event_type`,`event_phase_mask`,`event_chance`,`event_flags`,`event_param1`,`event_param2`,`event_param3`,`event_param4`,`event_param5`,`action_type`,`action_param1`,`action_param2`,`action_param3`,`action_param4`,`action_param5`,`action_param6`,`target_type`,`target_param1`,`target_param2`,`target_param3`,`target_param4`,`target_x`,`target_y`,`target_z`,`target_o`,`comment` SEPARATOR '\n'),256) FROM smart_scripts WHERE entryorguid=104307 AND source_type=0);
SET @stromkar_return_valkyr := (SELECT SHA2(GROUP_CONCAT(CONCAT_WS('|',COALESCE(CAST(`entryorguid` AS CHAR),'NULL'),COALESCE(CAST(`source_type` AS CHAR),'NULL'),COALESCE(CAST(`id` AS CHAR),'NULL'),COALESCE(CAST(`link` AS CHAR),'NULL'),COALESCE(CAST(`Difficulties` AS CHAR),'NULL'),COALESCE(CAST(`event_type` AS CHAR),'NULL'),COALESCE(CAST(`event_phase_mask` AS CHAR),'NULL'),COALESCE(CAST(`event_chance` AS CHAR),'NULL'),COALESCE(CAST(`event_flags` AS CHAR),'NULL'),COALESCE(CAST(`event_param1` AS CHAR),'NULL'),COALESCE(CAST(`event_param2` AS CHAR),'NULL'),COALESCE(CAST(`event_param3` AS CHAR),'NULL'),COALESCE(CAST(`event_param4` AS CHAR),'NULL'),COALESCE(CAST(`event_param5` AS CHAR),'NULL'),COALESCE(CAST(`action_type` AS CHAR),'NULL'),COALESCE(CAST(`action_param1` AS CHAR),'NULL'),COALESCE(CAST(`action_param2` AS CHAR),'NULL'),COALESCE(CAST(`action_param3` AS CHAR),'NULL'),COALESCE(CAST(`action_param4` AS CHAR),'NULL'),COALESCE(CAST(`action_param5` AS CHAR),'NULL'),COALESCE(CAST(`action_param6` AS CHAR),'NULL'),COALESCE(CAST(`target_type` AS CHAR),'NULL'),COALESCE(CAST(`target_param1` AS CHAR),'NULL'),COALESCE(CAST(`target_param2` AS CHAR),'NULL'),COALESCE(CAST(`target_param3` AS CHAR),'NULL'),COALESCE(CAST(`target_param4` AS CHAR),'NULL'),COALESCE(CAST(`target_x` AS CHAR),'NULL'),COALESCE(CAST(`target_y` AS CHAR),'NULL'),COALESCE(CAST(`target_z` AS CHAR),'NULL'),COALESCE(CAST(`target_o` AS CHAR),'NULL'),COALESCE(CAST(`comment` AS CHAR),'NULL')) ORDER BY `entryorguid`,`source_type`,`id`,`link`,`Difficulties`,`event_type`,`event_phase_mask`,`event_chance`,`event_flags`,`event_param1`,`event_param2`,`event_param3`,`event_param4`,`event_param5`,`action_type`,`action_param1`,`action_param2`,`action_param3`,`action_param4`,`action_param5`,`action_param6`,`target_type`,`target_param1`,`target_param2`,`target_param3`,`target_param4`,`target_x`,`target_y`,`target_z`,`target_o`,`comment` SEPARATOR '\n'),256) FROM smart_scripts WHERE entryorguid=93819 AND source_type=0);
SET @gaze_template := (SELECT SHA2(GROUP_CONCAT(CONCAT_WS('|',COALESCE(CAST(`entry` AS CHAR),'NULL'),COALESCE(CAST(`spellId` AS CHAR),'NULL'),COALESCE(CAST(`customEntry` AS CHAR),'NULL'),COALESCE(CAST(`VisualID` AS CHAR),'NULL'),COALESCE(CAST(`DecalPropertiesId` AS CHAR),'NULL'),COALESCE(CAST(`Radius` AS CHAR),'NULL'),COALESCE(CAST(`RadiusTarget` AS CHAR),'NULL'),COALESCE(CAST(`Height` AS CHAR),'NULL'),COALESCE(CAST(`HeightTarget` AS CHAR),'NULL'),COALESCE(CAST(`Float4` AS CHAR),'NULL'),COALESCE(CAST(`Float5` AS CHAR),'NULL'),COALESCE(CAST(`isMoving` AS CHAR),'NULL'),COALESCE(CAST(`Distance` AS CHAR),'NULL'),COALESCE(CAST(`Speed` AS CHAR),'NULL'),COALESCE(CAST(`RePatch` AS CHAR),'NULL'),COALESCE(CAST(`RePatchSpeed` AS CHAR),'NULL'),COALESCE(CAST(`MoveCurveID` AS CHAR),'NULL'),COALESCE(CAST(`ElapsedTime` AS CHAR),'NULL'),COALESCE(CAST(`MorphCurveID` AS CHAR),'NULL'),COALESCE(CAST(`FacingCurveID` AS CHAR),'NULL'),COALESCE(CAST(`ScaleCurveID` AS CHAR),'NULL'),COALESCE(CAST(`HasFollowsTerrain` AS CHAR),'NULL'),COALESCE(CAST(`HasAttached` AS CHAR),'NULL'),COALESCE(CAST(`HasAbsoluteOrientation` AS CHAR),'NULL'),COALESCE(CAST(`HasDynamicShape` AS CHAR),'NULL'),COALESCE(CAST(`HasFaceMovementDir` AS CHAR),'NULL'),COALESCE(CAST(`hasAreaTriggerBox` AS CHAR),'NULL'),COALESCE(CAST(`RollPitchYaw1X` AS CHAR),'NULL'),COALESCE(CAST(`RollPitchYaw1Y` AS CHAR),'NULL'),COALESCE(CAST(`RollPitchYaw1Z` AS CHAR),'NULL'),COALESCE(CAST(`TargetRollPitchYawX` AS CHAR),'NULL'),COALESCE(CAST(`TargetRollPitchYawY` AS CHAR),'NULL'),COALESCE(CAST(`TargetRollPitchYawZ` AS CHAR),'NULL'),COALESCE(CAST(`windX` AS CHAR),'NULL'),COALESCE(CAST(`windY` AS CHAR),'NULL'),COALESCE(CAST(`windZ` AS CHAR),'NULL'),COALESCE(CAST(`windSpeed` AS CHAR),'NULL'),COALESCE(CAST(`windType` AS CHAR),'NULL'),COALESCE(CAST(`polygon` AS CHAR),'NULL'),COALESCE(CAST(`comment` AS CHAR),'NULL')) ORDER BY `entry`,`spellId`,`customEntry`,`VisualID`,`DecalPropertiesId`,`Radius`,`RadiusTarget`,`Height`,`HeightTarget`,`Float4`,`Float5`,`isMoving`,`Distance`,`Speed`,`RePatch`,`RePatchSpeed`,`MoveCurveID`,`ElapsedTime`,`MorphCurveID`,`FacingCurveID`,`ScaleCurveID`,`HasFollowsTerrain`,`HasAttached`,`HasAbsoluteOrientation`,`HasDynamicShape`,`HasFaceMovementDir`,`hasAreaTriggerBox`,`RollPitchYaw1X`,`RollPitchYaw1Y`,`RollPitchYaw1Z`,`TargetRollPitchYawX`,`TargetRollPitchYawY`,`TargetRollPitchYawZ`,`windX`,`windY`,`windZ`,`windSpeed`,`windType`,`polygon`,`comment` SEPARATOR '\n'),256) FROM areatrigger_template WHERE entry=0 AND customEntry=11869);
SET @gaze_actions := (SELECT SHA2(GROUP_CONCAT(CONCAT_WS('|',COALESCE(CAST(`entry` AS CHAR),'NULL'),COALESCE(CAST(`customEntry` AS CHAR),'NULL'),COALESCE(CAST(`id` AS CHAR),'NULL'),COALESCE(CAST(`moment` AS CHAR),'NULL'),COALESCE(CAST(`actionType` AS CHAR),'NULL'),COALESCE(CAST(`targetFlags` AS CHAR),'NULL'),COALESCE(CAST(`spellId` AS CHAR),'NULL'),COALESCE(CAST(`maxCharges` AS CHAR),'NULL'),COALESCE(CAST(`hasAura` AS CHAR),'NULL'),COALESCE(CAST(`hasAura2` AS CHAR),'NULL'),COALESCE(CAST(`hasAura3` AS CHAR),'NULL'),COALESCE(CAST(`hasspell` AS CHAR),'NULL'),COALESCE(CAST(`chargeRecoveryTime` AS CHAR),'NULL'),COALESCE(CAST(`scaleStep` AS CHAR),'NULL'),COALESCE(CAST(`scaleMin` AS CHAR),'NULL'),COALESCE(CAST(`scaleMax` AS CHAR),'NULL'),COALESCE(CAST(`scaleVisualUpdate` AS CHAR),'NULL'),COALESCE(CAST(`hitMaxCount` AS CHAR),'NULL'),COALESCE(CAST(`amount` AS CHAR),'NULL'),COALESCE(CAST(`onDespawn` AS CHAR),'NULL'),COALESCE(CAST(`auraCaster` AS CHAR),'NULL'),COALESCE(CAST(`minDistance` AS CHAR),'NULL'),COALESCE(CAST(`comment` AS CHAR),'NULL')) ORDER BY `entry`,`customEntry`,`id`,`moment`,`actionType`,`targetFlags`,`spellId`,`maxCharges`,`hasAura`,`hasAura2`,`hasAura3`,`hasspell`,`chargeRecoveryTime`,`scaleStep`,`scaleMin`,`scaleMax`,`scaleVisualUpdate`,`hitMaxCount`,`amount`,`onDespawn`,`auraCaster`,`minDistance`,`comment` SEPARATOR '\n'),256) FROM areatrigger_actions WHERE entry=0 AND customEntry=11869);
SET @gaze_conditions := (SELECT SHA2(GROUP_CONCAT(CONCAT_WS('|',COALESCE(CAST(`SourceTypeOrReferenceId` AS CHAR),'NULL'),COALESCE(CAST(`SourceGroup` AS CHAR),'NULL'),COALESCE(CAST(`SourceEntry` AS CHAR),'NULL'),COALESCE(CAST(`SourceId` AS CHAR),'NULL'),COALESCE(CAST(`ElseGroup` AS CHAR),'NULL'),COALESCE(CAST(`ConditionTypeOrReference` AS CHAR),'NULL'),COALESCE(CAST(`ConditionTarget` AS CHAR),'NULL'),COALESCE(CAST(`ConditionValue1` AS CHAR),'NULL'),COALESCE(CAST(`ConditionValue2` AS CHAR),'NULL'),COALESCE(CAST(`ConditionValue3` AS CHAR),'NULL'),COALESCE(CAST(`NegativeCondition` AS CHAR),'NULL'),COALESCE(CAST(`ErrorTextId` AS CHAR),'NULL'),COALESCE(CAST(`ScriptName` AS CHAR),'NULL'),COALESCE(CAST(`Comment` AS CHAR),'NULL')) ORDER BY `SourceTypeOrReferenceId`,`SourceGroup`,`SourceEntry`,`SourceId`,`ElseGroup`,`ConditionTypeOrReference`,`ConditionTarget`,`ConditionValue1`,`ConditionValue2`,`ConditionValue3`,`NegativeCondition`,`ErrorTextId`,`ScriptName`,`Comment` SEPARATOR '\n'),256) FROM conditions WHERE SourceTypeOrReferenceId=26 AND SourceGroup=11869);
SET @stromkar_return_safe :=
 (SELECT COUNT(*) FROM creature_template WHERE entry=104307 AND AIName='SmartAI' AND ScriptName='npc_stromkar_thoradin')=1
 AND @stromkar_return_valkyr='a70d618f9c717d276a01bf6cba82d29bd910f2439c0b25ac288e481663001588'
 AND @gaze_template='30edad827fba3da512ee3d42e326f9ecf0096b082d13aa32d587c1894ddf814d'
 AND @gaze_actions='dace28f242dd6120bdfb507e91f0a1b449dbc208362c684ef177ebf25610d708'
 AND ((@stromkar_return_ghost='6d451d27cbf4f043681711e684b935789f8b43cf3191448e828a60c59d9867ae'
 AND (SELECT COUNT(*) FROM creature_template WHERE entry=93819 AND AIName='SmartAI' AND ScriptName='')=1
 AND (SELECT COUNT(*) FROM conditions WHERE SourceTypeOrReferenceId=26 AND SourceGroup=11869)=0)
 OR (@stromkar_return_ghost='efee1d4dafc6c843527159b1e17bf19af03870a5a826c2e6f5552119aaa45817'
 AND (SELECT COUNT(*) FROM creature_template WHERE entry=93819 AND AIName='SmartAI' AND ScriptName='npc_stromkar_valkyr')=1
 AND @gaze_conditions='cc781a853dbc5ed28302e68d253fb7e585a880157adcdace511fd2e20f1d5ab4'));
SET @stromkar_return_check := IF(COALESCE(@stromkar_return_safe,0),'DO 0','SIGNAL SQLSTATE ''45000'' SET MESSAGE_TEXT=''Stromkar native return actor/area/actions/conditions differ; review before promotion''');
PREPARE stromkar_return_check FROM @stromkar_return_check;EXECUTE stromkar_return_check;DEALLOCATE PREPARE stromkar_return_check;
UPDATE creature_template SET ScriptName='npc_stromkar_valkyr' WHERE @stromkar_return_safe AND entry=93819 AND ScriptName='';
INSERT INTO smart_scripts(entryorguid,source_type,id,event_type,event_chance,event_flags,event_param1,event_param2,action_type,action_param1,action_param2,target_type,comment)
SELECT 104307,0,6,52,100,1,3,104307,45,3,1,1,'Stromkar: offer native return only after final farewell text ends'
WHERE @stromkar_return_safe AND @stromkar_return_ghost='6d451d27cbf4f043681711e684b935789f8b43cf3191448e828a60c59d9867ae';
INSERT INTO conditions(SourceTypeOrReferenceId,SourceGroup,SourceEntry,SourceId,ElseGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2,ConditionValue3,NegativeCondition,ErrorTextId,ScriptName,Comment)
SELECT * FROM (
SELECT 26 AS SourceTypeOrReferenceId,11869 AS SourceGroup,0 AS SourceEntry,0 AS SourceId,0 AS ElseGroup,22 AS ConditionTypeOrReference,0 AS ConditionTarget,1539 AS ConditionValue1,0 AS ConditionValue2,0 AS ConditionValue3,1 AS NegativeCondition,0 AS ErrorTextId,'' AS ScriptName,'Stromkar: preserve shared gaze outside tomb' AS Comment
UNION ALL
SELECT 26,11869,0,0,1,36,0,0,0,0,0,0,'','Stromkar: native return after real final credit and owned weapon'
UNION ALL
SELECT 26,11869,0,0,1,31,0,3,93819,0,0,0,'','Stromkar: native return after real final credit and owned weapon'
UNION ALL
SELECT 26,11869,0,0,1,22,0,1539,0,0,0,0,'','Stromkar: native return after real final credit and owned weapon'
UNION ALL
SELECT 26,11869,0,0,1,44,0,1037,10,0,0,0,'','Stromkar: native return after real final credit and owned weapon'
UNION ALL
SELECT 26,11869,0,0,1,36,1,0,0,0,0,0,'','Stromkar: native return after real final credit and owned weapon'
UNION ALL
SELECT 26,11869,0,0,1,15,1,1,0,0,0,0,'','Stromkar: native return after real final credit and owned weapon'
UNION ALL
SELECT 26,11869,0,0,1,2,1,128910,1,1,0,0,'','Stromkar: native return after real final credit and owned weapon'
UNION ALL
SELECT 26,11869,0,0,1,9,1,41105,0,0,0,0,'','Stromkar: native return after real final credit and owned weapon'
UNION ALL
SELECT 26,11869,0,0,1,8,1,41105,0,0,1,0,'','Stromkar: native return after real final credit and owned weapon'
UNION ALL
SELECT 26,11869,0,0,1,41,1,41105,103114,1,0,0,'condition_stromkar_return','Stromkar: native return after real final credit and owned weapon'
UNION ALL
SELECT 26,11869,0,0,2,36,0,0,0,0,0,0,'','Stromkar: native return after real final credit and owned weapon'
UNION ALL
SELECT 26,11869,0,0,2,31,0,3,93819,0,0,0,'','Stromkar: native return after real final credit and owned weapon'
UNION ALL
SELECT 26,11869,0,0,2,22,0,1539,0,0,0,0,'','Stromkar: native return after real final credit and owned weapon'
UNION ALL
SELECT 26,11869,0,0,2,44,0,1037,10,0,0,0,'','Stromkar: native return after real final credit and owned weapon'
UNION ALL
SELECT 26,11869,0,0,2,36,1,0,0,0,0,0,'','Stromkar: native return after real final credit and owned weapon'
UNION ALL
SELECT 26,11869,0,0,2,15,1,1,0,0,0,0,'','Stromkar: native return after real final credit and owned weapon'
UNION ALL
SELECT 26,11869,0,0,2,2,1,128910,1,1,0,0,'','Stromkar: native return after real final credit and owned weapon'
UNION ALL
SELECT 26,11869,0,0,2,28,1,41105,0,0,0,0,'','Stromkar: native return after real final credit and owned weapon'
UNION ALL
SELECT 26,11869,0,0,2,8,1,41105,0,0,1,0,'','Stromkar: native return after real final credit and owned weapon'
UNION ALL
SELECT 26,11869,0,0,2,41,1,41105,103114,1,0,0,'condition_stromkar_return','Stromkar: native return after real final credit and owned weapon'
) AS native_return WHERE @stromkar_return_safe AND @gaze_conditions IS NULL;
COMMIT;
SET SESSION group_concat_max_len=@stromkar_return_concat;
