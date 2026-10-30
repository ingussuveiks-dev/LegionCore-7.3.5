-- Code-coupled: promote the registered AT controller before enabling this data.
-- Purely cosmetic, two-second projection of the original shadow797909 texture.
START TRANSACTION;
SET @ground_templates=(SELECT COUNT(*) FROM areatrigger_template WHERE customEntry=900001 OR entry=900001 OR spellId=900001);
SET @ground_scripts=(SELECT COUNT(*) FROM areatrigger_scripts WHERE entry=900001 OR ScriptName='at_stromkar_ground_cracks');
SET @ground_safe=((@ground_templates=0 AND @ground_scripts=0) OR (@ground_templates=1 AND @ground_scripts=1))
 AND NOT EXISTS(SELECT 1 FROM areatrigger_template WHERE (customEntry=900001 OR entry=900001 OR spellId=900001)
 AND NOT IFNULL((`entry`=0 AND
 `spellId`=0 AND
 `customEntry`=900001 AND
 `VisualID`=0 AND
 `DecalPropertiesId`=10000 AND
 ABS(`Radius`-14.7181)<0.00001 AND
 ABS(`RadiusTarget`-14.7181)<0.00001 AND
 `Height`=0 AND
 `HeightTarget`=0 AND
 `Float4`=0 AND
 `Float5`=0 AND
 `isMoving`=0 AND
 `Distance`=0 AND
 `Speed`=0 AND
 `RePatch`=0 AND
 `RePatchSpeed`=0 AND
 `MoveCurveID`=0 AND
 `ElapsedTime`=0 AND
 `MorphCurveID`=0 AND
 `FacingCurveID`=0 AND
 `ScaleCurveID`=0 AND
 `HasFollowsTerrain`=1 AND
 `HasAttached`=0 AND
 `HasAbsoluteOrientation`=0 AND
 `HasDynamicShape`=0 AND
 `HasFaceMovementDir`=0 AND
 `hasAreaTriggerBox`=0 AND
 `RollPitchYaw1X`=0 AND
 `RollPitchYaw1Y`=0 AND
 `RollPitchYaw1Z`=0 AND
 `TargetRollPitchYawX`=0 AND
 `TargetRollPitchYawY`=0 AND
 `TargetRollPitchYawZ`=0 AND
 `windX`=0 AND
 `windY`=0 AND
 `windZ`=0 AND
 `windSpeed`=0 AND
 `windType`=0 AND
 `polygon`=0 AND
 `comment`='Stromkar: original shadow crack texture projected on the floor'),0))
 AND NOT EXISTS(SELECT 1 FROM areatrigger_scripts WHERE (entry=900001 OR ScriptName='at_stromkar_ground_cracks')
 AND NOT IFNULL((entry=900001 AND ScriptName='at_stromkar_ground_cracks'),0))
 AND NOT EXISTS(SELECT 1 FROM areatrigger_actions WHERE entry=900001 OR customEntry=900001)
 AND NOT EXISTS(SELECT 1 FROM areatrigger_data WHERE entry=900001 OR customEntry=900001 OR spellId=900001)
 AND NOT EXISTS(SELECT 1 FROM areatrigger_polygon WHERE entry=900001 OR spellId=900001)
 AND NOT EXISTS(SELECT 1 FROM areatrigger_template_circle WHERE entry=900001 OR spellId=900001)
 AND NOT EXISTS(SELECT 1 FROM areatrigger_template_sequence WHERE entry=900001 OR spellId=900001);
SET @ground_guard=IF(@ground_safe,'DO 0',
 'SIGNAL SQLSTATE ''45000'' SET MESSAGE_TEXT=''Stromkar projected crack inputs differ; review before applying''');
PREPARE ground_check FROM @ground_guard;
EXECUTE ground_check;
DEALLOCATE PREPARE ground_check;
INSERT INTO areatrigger_template(`entry`,`spellId`,`customEntry`,`VisualID`,`DecalPropertiesId`,`Radius`,`RadiusTarget`,`Height`,`HeightTarget`,`Float4`,`Float5`,`isMoving`,`Distance`,`Speed`,`RePatch`,`RePatchSpeed`,`MoveCurveID`,`ElapsedTime`,`MorphCurveID`,`FacingCurveID`,`ScaleCurveID`,`HasFollowsTerrain`,`HasAttached`,`HasAbsoluteOrientation`,`HasDynamicShape`,`HasFaceMovementDir`,`hasAreaTriggerBox`,`RollPitchYaw1X`,`RollPitchYaw1Y`,`RollPitchYaw1Z`,`TargetRollPitchYawX`,`TargetRollPitchYawY`,`TargetRollPitchYawZ`,`windX`,`windY`,`windZ`,`windSpeed`,`windType`,`polygon`,`comment`)
SELECT 0,0,900001,0,10000,14.7181,14.7181,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,'Stromkar: original shadow crack texture projected on the floor'
WHERE @ground_safe AND @ground_templates=0;
INSERT INTO areatrigger_scripts(`entry`,`ScriptName`)
SELECT 900001,'at_stromkar_ground_cracks'
WHERE @ground_safe AND @ground_scripts=0;
COMMIT;
