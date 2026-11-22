START TRANSACTION;

-- Ten 26365 placements from TrinityCore's archived 2018_05_21_00_world.sql, offsets 1931 through 1940.
-- Use the phase mask already shared by the original Tobias and Coffer in this 7.3.5 world.
INSERT IGNORE INTO `creature`
    (`guid`, `id`, `map`, `zoneId`, `areaId`, `spawnMask`, `phaseMask`, `equipment_id`,
     `position_x`, `position_y`, `position_z`, `orientation`, `spawntimesecs`)
VALUES
    (3300100, 38022, 654, 4714, 4730, 1, 16384, 1, -2147.919, 1624.259, -43.0625, 4.916778, 30),
    (3300101, 38022, 654, 4714, 4730, 1, 16384, 1, -2142.167, 1630.174, -42.47384, 3.071779, 30),
    (3300102, 38022, 654, 4714, 4730, 1, 16384, 1, -2139.622, 1621.071, -43.42087, 3.612832, 30),
    (3300103, 38022, 654, 4714, 4730, 1, 16384, 1, -2125.575, 1625.254, -42.15344, 3.612832, 30),
    (3300104, 38022, 654, 4714, 4730, 1, 16384, 1, -2128.738, 1614.806, -43.49768, 3.612832, 30),
    (3300105, 38022, 654, 4714, 4730, 1, 16384, 1, -2131.174, 1617.95, -43.49754, 3.612832, 30),
    (3300106, 38022, 654, 4714, 4730, 1, 16384, 1, -2123.288, 1610.599, -43.49564, 5.044002, 30),
    (3300107, 38022, 654, 4714, 4730, 1, 16384, 1, -2116.068, 1615.49, -43.34274, 4.869469, 30),
    (3300108, 38022, 654, 4714, 4730, 1, 16384, 1, -2129.976, 1606.859, -43.49542, 4.118977, 30),
    (3300109, 38022, 654, 4714, 4730, 1, 16384, 1, -2112.467, 1618.946, -43.03592, 5.183628, 30);

UPDATE `creature_template` SET `AIName`='SmartAI' WHERE `entry`=38022 AND `AIName`='' AND `ScriptName`='';

-- Original Gilneas and Undercity faction templates are mutually hostile in the extracted 7.3.5 DB2.
UPDATE `creature_template` SET `faction`=2213 WHERE `entry`=38022 AND `faction`=16;
UPDATE `creature_template` SET `faction`=2207 WHERE `entry`=38027 AND `faction`=2134;

-- Matching-era implementation documents Shoot and Black Shot. Keep ordinary victim and combat movement.
INSERT IGNORE INTO `smart_scripts`
    (`entryorguid`, `source_type`, `id`, `event_type`, `event_chance`, `event_param1`, `event_param2`,
     `event_param3`, `event_param4`, `action_type`, `action_param1`, `action_param2`, `target_type`, `comment`)
VALUES
    (38022, 0, 0, 0, 100, 0, 0, 2300, 3900, 11, 6660, 64, 2, 'Veteran Dark Ranger - In combat - Shoot'),
    (38022, 0, 1, 0, 100, 8500, 12000, 18500, 24000, 11, 75439, 0, 2, 'Veteran Dark Ranger - In combat - Black Shot');

INSERT IGNORE INTO `conditions`
    (`SourceTypeOrReferenceId`, `SourceGroup`, `SourceEntry`, `ConditionTypeOrReference`, `ConditionTarget`,
     `ConditionValue1`, `ConditionValue2`, `Comment`)
VALUES
    (13, 1, 71011, 31, 0, 3, 38022, 'Tobias rally selects Veteran Dark Rangers'),
    (13, 1, 71018, 31, 0, 3, 38027, 'Tobias retreat selects Tal\'doren Trackers');

UPDATE `gameobject_loot_template` SET `QuestRequired`=1
WHERE `Entry`=201939 AND `Item`=50086 AND `Currency`=0 AND `Reference`=0;

UPDATE `gameobject` SET `spawntimesecs`=5
WHERE `guid`=47222 AND `id`=201939 AND `map`=654 AND `spawntimesecs`=300;

COMMIT;
