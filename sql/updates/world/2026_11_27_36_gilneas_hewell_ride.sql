-- Archived Stout Mountain Horse route, numbered from one for this SmartAI loader.
CREATE TEMPORARY TABLE `hewell_route` LIKE `waypoints`;
INSERT INTO `hewell_route` (`entry`,`pointid`,`position_x`,`position_y`,`position_z`) VALUES
    (3876500,1,-2046.791138,965.273254,70.093262),
    (3876500,2,-2052.965820,951.965210,70.019661),
    (3876500,3,-2044.199829,943.589844,69.623657),
    (3876500,4,-2016.786865,937.619995,63.961536),
    (3876500,5,-2000.785767,922.673584,58.993900),
    (3876500,6,-1994.652954,901.187744,57.168518),
    (3876500,7,-1990.477905,886.340149,58.822002),
    (3876500,8,-1984.289673,876.254028,62.284023),
    (3876500,9,-1976.330811,865.808594,64.363052),
    (3876500,10,-1949.194336,857.085693,64.791534),
    (3876500,11,-1894.699829,862.718079,80.235916),
    (3876500,12,-1882.078491,876.898865,82.060173),
    (3876500,13,-1872.024292,949.170898,78.025955),
    (3876500,14,-1889.070923,981.427612,75.891670),
    (3876500,15,-1883.198853,992.290039,74.211685),
    (3876500,16,-1880.172485,1030.361938,65.171577),
    (3876500,17,-1854.369629,1067.181519,55.395538),
    (3876500,18,-1849.193237,1106.659668,45.402390),
    (3876500,19,-1842.725708,1139.716919,37.483692),
    (3876500,20,-1858.252319,1212.784546,23.266125),
    (3876500,21,-1855.603149,1219.384644,22.489773),
    (3876500,22,-1847.629395,1217.708496,21.653622),
    (3876500,23,-1822.004761,1193.337769,19.576681),
    (3876500,24,-1789.048828,1172.176514,21.386976),
    (3876500,25,-1744.401123,1158.758667,26.958891),
    (3876500,26,-1721.482300,1148.631470,26.787212),
    (3876500,27,-1693.076660,1139.284546,25.749735),
    (3876500,28,-1655.684326,1114.898315,27.912958),
    (3876500,29,-1630.810059,1110.211670,29.174562),
    (3876500,30,-1568.839600,1123.369873,30.205023),
    (3876500,31,-1534.276855,1115.624146,30.150204),
    (3876500,32,-1502.919434,1086.336914,29.039797),
    (3876500,33,-1472.096680,1080.557251,29.653278),
    (3876500,34,-1437.718262,1078.914917,29.179899),
    (3876500,35,-1400.402954,1090.951416,29.642042),
    (3876500,36,-1364.364624,1115.645996,31.042089),
    (3876500,37,-1316.513062,1124.131348,37.065750),
    (3876500,38,-1315.350830,1131.431519,37.209011),
    (3876500,39,-1354.291748,1175.771973,34.790340),
    (3876500,40,-1370.140015,1192.152100,35.389881),
    (3876500,41,-1384.801392,1207.186890,35.557831),
    (3876500,42,-1378.258911,1213.986328,35.557831);

CREATE TEMPORARY TABLE `hewell_events` LIKE `smart_scripts`;
INSERT INTO `hewell_events`
    (`entryorguid`,`source_type`,`id`,`link`,`event_type`,`event_flags`,`event_param1`,`event_param2`,
     `action_type`,`action_param1`,`action_param2`,`target_type`,`comment`)
VALUES
    (38764,0,0,1,62,0,11079,0,11,72773,2,7,'Lord Hewell - Ask for a horse - Forcecast the native summon on the player'),
    (38764,0,1,0,61,0,0,0,72,0,0,7,'Lord Hewell - Close the gossip after choosing the ride'),
    (38765,0,0,0,27,1,0,0,53,1,3876500,7,'Stout Mountain Horse - Owner boarded - Run the Livery Outpost route'),
    (38765,0,1,0,58,1,42,3876500,11,51254,2,1,'Stout Mountain Horse - At Livery Outpost - Eject the rider'),
    (38765,0,2,0,28,1,0,0,41,250,0,1,'Stout Mountain Horse - Owner dismounted - Despawn');

CREATE TEMPORARY TABLE `hewell_conditions` LIKE `conditions`;
INSERT INTO `hewell_conditions`
    (`SourceTypeOrReferenceId`,`SourceGroup`,`SourceEntry`,`SourceId`,`ElseGroup`,`ConditionTypeOrReference`,
     `ConditionTarget`,`ConditionValue1`,`ConditionValue2`,`NegativeCondition`,`Comment`)
VALUES
    (15,11079,0,0,0,28,0,24677,0,0,'Lord Hewell - Show the ride to an active Flank the Forsaken player'),
    (15,11079,0,0,0,56,0,0,0,1,'Lord Hewell - Show the ride while not on a transport'),
    (15,11079,0,0,0,64,0,0,0,1,'Lord Hewell - Show the ride out of combat'),
    (22,1,38764,0,0,28,0,24677,0,0,'Lord Hewell - Require actual Flank the Forsaken for gossip selection'),
    (22,1,38764,0,0,56,0,0,0,1,'Lord Hewell - Do not start a second ride on a transport'),
    (22,1,38764,0,0,64,0,0,0,1,'Lord Hewell - Do not start a ride in combat'),
    (22,1,38765,0,0,33,1,0,6,0,'Stout Mountain Horse - Path starts only for its summoner'),
    (22,3,38765,0,0,33,1,0,6,0,'Stout Mountain Horse - Cleanup belongs to its summoner');

START TRANSACTION;

SET @hewell_safe :=
    EXISTS (SELECT 1 FROM `creature` WHERE `guid`=149553 AND `id`=38764 AND `map`=654 AND `areaId`=4788)
    AND EXISTS (SELECT 1 FROM `creature_template` WHERE `entry`=38764 AND `gossip_menu_id` IN(0,11079)
        AND `npcflag` IN(0,1) AND `AIName` IN('','SmartAI') AND `ScriptName`='')
    AND EXISTS (SELECT 1 FROM `creature_template` WHERE `entry`=38765 AND `VehicleId` IN(0,542)
        AND `AIName` IN('','SmartAI') AND `ScriptName`='')
    AND EXISTS (SELECT 1 FROM `gossip_menu` WHERE `MenuID`=11079 AND `TextID`=15408)
    AND EXISTS (SELECT 1 FROM `gossip_menu_option` WHERE `MenuID`=11079 AND `OptionID`=0 AND `OptionBroadcastTextID`=38635)
    AND EXISTS (SELECT 1 FROM `quest_template_addon` WHERE `ID`=24677 AND `PrevQuestID`=24592 AND `NextQuestID`=24575)
    AND NOT EXISTS (SELECT 1 FROM `spell_script_names` WHERE `spell_id`=51254 AND `ScriptName`<>'spell_gen_eject_all_passengers')
    AND NOT EXISTS (SELECT 1 FROM `waypoints` a LEFT JOIN `hewell_route` e USING(`entry`,`pointid`)
        WHERE a.`entry`=3876500 AND (e.`entry` IS NULL OR (a.`position_x`,a.`position_y`,a.`position_z`)<>(e.`position_x`,e.`position_y`,e.`position_z`)))
    AND NOT EXISTS (SELECT 1 FROM `smart_scripts` a LEFT JOIN `hewell_events` e USING(`entryorguid`,`source_type`,`id`)
        WHERE a.`source_type`=0 AND a.`entryorguid` IN(38764,38765,-149553) AND (e.`entryorguid` IS NULL OR
            (a.`link`,a.`event_type`,a.`event_flags`,a.`event_param1`,a.`event_param2`,a.`action_type`,a.`action_param1`,a.`action_param2`,a.`target_type`)
            <>(e.`link`,e.`event_type`,e.`event_flags`,e.`event_param1`,e.`event_param2`,e.`action_type`,e.`action_param1`,e.`action_param2`,e.`target_type`)))
    AND NOT EXISTS (SELECT 1 FROM `conditions` a LEFT JOIN `hewell_conditions` e
        USING(`SourceTypeOrReferenceId`,`SourceGroup`,`SourceEntry`,`SourceId`,`ElseGroup`,`ConditionTypeOrReference`,`ConditionTarget`,`ConditionValue1`,`ConditionValue2`,`ConditionValue3`)
        WHERE ((a.`SourceTypeOrReferenceId`=15 AND a.`SourceGroup`=11079 AND a.`SourceEntry`=0)
            OR (a.`SourceTypeOrReferenceId`=22 AND a.`SourceEntry` IN(38764,38765,-149553)))
        AND (e.`SourceTypeOrReferenceId` IS NULL OR a.`NegativeCondition`<>e.`NegativeCondition` OR a.`ScriptName`<>''));

SET @hewell_guard := IF(@hewell_safe, 'DO 0',
    'SIGNAL SQLSTATE ''45000'' SET MESSAGE_TEXT = ''Lord Hewell ride: native gossip, vehicle or route context differs''');
PREPARE hewell_check FROM @hewell_guard;
EXECUTE hewell_check;
DEALLOCATE PREPARE hewell_check;

UPDATE `creature_template` SET `gossip_menu_id`=11079,`npcflag`=1,`AIName`='SmartAI'
WHERE @hewell_safe AND `entry`=38764;

UPDATE `creature_template` SET `VehicleId`=542,`AIName`='SmartAI'
WHERE @hewell_safe AND `entry`=38765;

INSERT INTO `waypoints` SELECT e.* FROM `hewell_route` e
WHERE @hewell_safe AND NOT EXISTS (SELECT 1 FROM `waypoints` a WHERE a.`entry`=e.`entry` AND a.`pointid`=e.`pointid`);

INSERT INTO `smart_scripts` SELECT e.* FROM `hewell_events` e
WHERE @hewell_safe AND NOT EXISTS (SELECT 1 FROM `smart_scripts` a WHERE a.`entryorguid`=e.`entryorguid` AND a.`source_type`=e.`source_type` AND a.`id`=e.`id`);

INSERT INTO `conditions` SELECT e.* FROM `hewell_conditions` e
WHERE @hewell_safe AND NOT EXISTS (SELECT 1 FROM `conditions` a WHERE (a.`SourceTypeOrReferenceId`,a.`SourceGroup`,a.`SourceEntry`,a.`SourceId`,a.`ElseGroup`,a.`ConditionTypeOrReference`,a.`ConditionTarget`,a.`ConditionValue1`,a.`ConditionValue2`,a.`ConditionValue3`)
    =(e.`SourceTypeOrReferenceId`,e.`SourceGroup`,e.`SourceEntry`,e.`SourceId`,e.`ElseGroup`,e.`ConditionTypeOrReference`,e.`ConditionTarget`,e.`ConditionValue1`,e.`ConditionValue2`,e.`ConditionValue3`));

INSERT INTO `spell_script_names` (`spell_id`,`ScriptName`)
SELECT 51254,'spell_gen_eject_all_passengers' WHERE @hewell_safe
AND NOT EXISTS (SELECT 1 FROM `spell_script_names` WHERE `spell_id`=51254 AND `ScriptName`='spell_gen_eject_all_passengers');

COMMIT;
DROP TEMPORARY TABLE `hewell_conditions`, `hewell_events`, `hewell_route`;
