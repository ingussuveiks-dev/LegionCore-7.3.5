-- Requires loaded native DecalProperties/TextureBlendSet stores in matching EXE.
-- Custom adaptation of native24/21. Preserve every original record.
START TRANSACTION;
SET @ground_decal=(SELECT COUNT(*) FROM decal_properties WHERE ID=10000);
SET @ground_blend=(SELECT COUNT(*) FROM texture_blend_set WHERE ID=10000);
SET @ground_notices=(SELECT COUNT(*) FROM hotfix_data WHERE RecordID=10000 AND TableHash IN(3350717493,2512539351));
SELECT COALESCE(MAX(Id),0)+1 INTO @ground_notice_id FROM hotfix_data FOR UPDATE;
SET @ground_safe=((@ground_decal=0 AND @ground_blend=0 AND @ground_notices=0)
 OR (@ground_decal=1 AND @ground_blend=1 AND @ground_notices=2))
 AND (@ground_notices=2 OR @ground_notice_id BETWEEN 1 AND 2147483646)
 AND NOT EXISTS(SELECT 1 FROM decal_properties WHERE ID=10000 AND NOT IFNULL((`ID`=10000 AND
 `FileDataID`=0 AND
 `InnerRadius`=0.5 AND
 `OuterRadius`=0.5 AND
 `Rim`=0 AND
 `Gain`=4 AND
 `ModX`=4 AND
 `Scale`=1 AND
 `FadeIn`=0.5 AND
 `FadeOut`=0.5 AND
 `Priority`=75 AND
 `BlendMode`=0 AND
 `TopTextureBlendSetID`=10000 AND
 `BotTextureBlendSetID`=10000 AND
 `GameFlags`=0 AND
 `Flags`=0 AND
 `CasterDecalPropertiesID`=0 AND
 `VerifiedBuild`=26972),0))
 AND NOT EXISTS(SELECT 1 FROM texture_blend_set WHERE ID=10000 AND NOT IFNULL((`ID`=10000 AND
 `TextureFileDataID1`=797909 AND
 `TextureFileDataID2`=1002607 AND
 `TextureFileDataID3`=1002607 AND
 `TextureScrollRateU1`=0 AND
 `TextureScrollRateU2`=0 AND
 `TextureScrollRateU3`=0 AND
 `TextureScrollRateV1`=0 AND
 `TextureScrollRateV2`=0 AND
 `TextureScrollRateV3`=0 AND
 `TextureScaleU1`=1 AND
 `TextureScaleU2`=1 AND
 `TextureScaleU3`=1 AND
 `TextureScaleV1`=1 AND
 `TextureScaleV2`=1 AND
 `TextureScaleV3`=1 AND
 `ModX1`=4 AND
 `ModX2`=4 AND
 `ModX3`=4 AND
 `ModX4`=4 AND
 `SwizzleRed`=0 AND
 `SwizzleGreen`=1 AND
 `SwizzleBlue`=2 AND
 `SwizzleAlpha`=3 AND
 `VerifiedBuild`=26972),0))
 AND NOT EXISTS(SELECT 1 FROM hotfix_data WHERE RecordID=10000 AND TableHash IN(3350717493,2512539351)
 AND (Deleted<>0 OR Timestamp<>0 OR Id=0))
 AND (@ground_notices=0 OR ((SELECT COUNT(*) FROM hotfix_data WHERE RecordID=10000 AND TableHash=3350717493)=1
 AND (SELECT COUNT(*) FROM hotfix_data WHERE RecordID=10000 AND TableHash=2512539351)=1));
SET @ground_guard=IF(@ground_safe,'DO 0',
 'SIGNAL SQLSTATE ''45000'' SET MESSAGE_TEXT=''Stromkar projected crack inputs differ; review before applying''');
PREPARE ground_check FROM @ground_guard;
EXECUTE ground_check;
DEALLOCATE PREPARE ground_check;
INSERT INTO decal_properties(`ID`,`FileDataID`,`InnerRadius`,`OuterRadius`,`Rim`,`Gain`,`ModX`,`Scale`,`FadeIn`,`FadeOut`,`Priority`,`BlendMode`,`TopTextureBlendSetID`,`BotTextureBlendSetID`,`GameFlags`,`Flags`,`CasterDecalPropertiesID`,`VerifiedBuild`)
SELECT 10000,0,0.5,0.5,0,4,4,1,0.5,0.5,75,0,10000,10000,0,0,0,26972
WHERE @ground_safe AND @ground_decal=0;
INSERT INTO texture_blend_set(`ID`,`TextureFileDataID1`,`TextureFileDataID2`,`TextureFileDataID3`,`TextureScrollRateU1`,`TextureScrollRateU2`,`TextureScrollRateU3`,`TextureScrollRateV1`,`TextureScrollRateV2`,`TextureScrollRateV3`,`TextureScaleU1`,`TextureScaleU2`,`TextureScaleU3`,`TextureScaleV1`,`TextureScaleV2`,`TextureScaleV3`,`ModX1`,`ModX2`,`ModX3`,`ModX4`,`SwizzleRed`,`SwizzleGreen`,`SwizzleBlue`,`SwizzleAlpha`,`VerifiedBuild`)
SELECT 10000,797909,1002607,1002607,0,0,0,0,0,0,1,1,1,1,1,1,4,4,4,4,0,1,2,3,26972
WHERE @ground_safe AND @ground_blend=0;
INSERT INTO hotfix_data(Id,TableHash,RecordID,Timestamp,Deleted)
SELECT @ground_notice_id,3350717493,10000,0,0 WHERE @ground_safe AND @ground_notices=0
UNION ALL SELECT @ground_notice_id+1,2512539351,10000,0,0 WHERE @ground_safe AND @ground_notices=0;
COMMIT;
