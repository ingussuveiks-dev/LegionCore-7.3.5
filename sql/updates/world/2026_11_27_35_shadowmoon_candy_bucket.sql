-- Quest 12409 is the shared Hallow's End reward in the two Shadowmoon inns.
-- The 23877 quest POI matches the archived 190115 Altar of Sha'tar placement.
START TRANSACTION;

SET @shadowmoon_candy_safe :=
    EXISTS (SELECT 1 FROM `quest_template` WHERE `ID`=12409 AND `QuestSortID`=-21 AND `RewardItem1`=37586 AND `RewardAmount1`=1)
    AND EXISTS (SELECT 1 FROM `game_event_seasonal_questrelation` WHERE `questId`=12409 AND `eventEntry`=12)
    AND EXISTS (SELECT 1 FROM `gameobject_template` WHERE `entry`=190115 AND `type`=2 AND `Data1`=8937)
    AND EXISTS (SELECT 1 FROM `gameobject_template` WHERE `entry`=190116 AND `type`=2 AND `Data1`=8938)
    AND EXISTS (SELECT 1 FROM `gameobject` g JOIN `game_event_gameobject` e USING (`guid`)
        WHERE g.`guid`=75463 AND g.`id`=190116 AND g.`map`=530 AND g.`areaId`=3938 AND e.`eventEntry`=12)
    AND EXISTS (SELECT 1 FROM `gameobject_queststarter` WHERE `id`=190116 AND `quest`=12409)
    AND (SELECT COUNT(*) FROM `gameobject_questender` WHERE `id` IN(190115,190116) AND `quest`=12409)=2
    AND NOT EXISTS (SELECT 1 FROM `gameobject_queststarter` WHERE `id`=190115 AND `quest`<>12409)
    AND NOT EXISTS (SELECT 1 FROM `gameobject` WHERE (`guid`=3101075 OR `id`=190115)
        AND NOT (`guid`=3101075 AND `id`=190115 AND `map`=530 AND `zoneId`=3520 AND `areaId`=3754
            AND `spawnMask`=1 AND `phaseMask`=1 AND ABS(`position_x`+2980.84)<0.01
            AND ABS(`position_y`-870.225)<0.01 AND ABS(`position_z`+7.47893)<0.01))
    AND NOT EXISTS (SELECT 1 FROM `game_event_gameobject` WHERE `guid`=3101075 AND `eventEntry`<>12)
    AND NOT EXISTS (SELECT 1 FROM `conditions` WHERE `SourceTypeOrReferenceId`=19 AND `SourceEntry`=12409
        AND NOT (`SourceGroup`=0 AND `SourceId`=0 AND `ConditionTarget`=0 AND `NegativeCondition`=0
            AND `ConditionValue3`=0 AND `ScriptName`='' AND (
                (`ElseGroup`,`ConditionTypeOrReference`,`ConditionValue1`,`ConditionValue2`) IN
                ((0,12,12,0),(0,23,3754,0),(0,5,932,248),(1,12,12,0),(1,23,3938,0),(1,5,934,248)))));

SET @shadowmoon_candy_guard := IF(@shadowmoon_candy_safe, 'DO 0',
    'SIGNAL SQLSTATE ''45000'' SET MESSAGE_TEXT = ''Shadowmoon Candy Bucket: native quest, placement or holiday context differs''');
PREPARE shadowmoon_candy_check FROM @shadowmoon_candy_guard;
EXECUTE shadowmoon_candy_check;
DEALLOCATE PREPARE shadowmoon_candy_check;

INSERT INTO `gameobject`
    (`guid`,`id`,`map`,`zoneId`,`areaId`,`spawnMask`,`phaseMask`,`position_x`,`position_y`,`position_z`,`orientation`,
     `rotation0`,`rotation1`,`rotation2`,`rotation3`,`spawntimesecs`,`animprogress`,`state`)
SELECT 3101075,190115,530,3520,3754,1,1,-2980.84,870.225,-7.47893,0.069813,0,0,0.0348994,0.999391,180,100,1
WHERE @shadowmoon_candy_safe AND NOT EXISTS (SELECT 1 FROM `gameobject` WHERE `guid`=3101075);

INSERT INTO `game_event_gameobject` (`eventEntry`,`guid`)
SELECT 12,3101075 WHERE @shadowmoon_candy_safe
AND NOT EXISTS (SELECT 1 FROM `game_event_gameobject` WHERE `eventEntry`=12 AND `guid`=3101075);

INSERT INTO `gameobject_queststarter` (`id`,`quest`)
SELECT 190115,12409 WHERE @shadowmoon_candy_safe
AND NOT EXISTS (SELECT 1 FROM `gameobject_queststarter` WHERE `id`=190115 AND `quest`=12409);

INSERT INTO `conditions`
    (`SourceTypeOrReferenceId`,`SourceGroup`,`SourceEntry`,`SourceId`,`ElseGroup`,`ConditionTypeOrReference`,
     `ConditionTarget`,`ConditionValue1`,`ConditionValue2`,`ConditionValue3`,`NegativeCondition`,`ErrorTextId`,`ScriptName`,`Comment`)
SELECT 19,0,12409,0,r.variant,r.condition_type,0,r.value1,r.value2,0,0,0,'',r.description
FROM (
    SELECT 0 AS variant,12 AS condition_type,12 AS value1,0 AS value2,'Shadowmoon Candy Bucket - Hallow\'s End at Altar of Sha\'tar' AS description
    UNION ALL SELECT 0,23,3754,0,'Shadowmoon Candy Bucket - Altar of Sha\'tar'
    UNION ALL SELECT 0,5,932,248,'Shadowmoon Candy Bucket - neutral or better with the Aldor'
    UNION ALL SELECT 1,12,12,0,'Shadowmoon Candy Bucket - Hallow\'s End at Sanctum of the Stars'
    UNION ALL SELECT 1,23,3938,0,'Shadowmoon Candy Bucket - Sanctum of the Stars'
    UNION ALL SELECT 1,5,934,248,'Shadowmoon Candy Bucket - neutral or better with the Scryers'
) r
WHERE @shadowmoon_candy_safe AND NOT EXISTS (SELECT 1 FROM `conditions` c
    WHERE c.`SourceTypeOrReferenceId`=19 AND c.`SourceEntry`=12409 AND c.`ElseGroup`=r.variant
        AND c.`ConditionTypeOrReference`=r.condition_type AND c.`ConditionValue1`=r.value1 AND c.`ConditionValue2`=r.value2);

COMMIT;
