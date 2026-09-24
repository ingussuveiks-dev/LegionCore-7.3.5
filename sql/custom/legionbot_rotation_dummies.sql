-- Seven GM Island test dummies in three isolated groups: 1, 2 and 4 targets.
-- Check that creature entries 900901/900902/900904 and spawn GUIDs
-- 900901001-900901007 are unused before applying to another world database.
-- Positions were sampled from the extracted Legion 7.3.5 map 1 terrain.
-- Each dummy must be tagged by a real player before it counts as an attacker
-- for defensive LegionBotAI. Its script keeps it stationary and damage-free.
USE `legion_world`;

INSERT IGNORE INTO `creature_template`
    (`entry`, `minlevel`, `maxlevel`, `faction`, `unit_flags`, `ScriptName`)
VALUES
    (900901, 110, 110, 7, 131072, 'npc_legionbot_rotation_dummy'),
    (900902, 110, 110, 7, 131072, 'npc_legionbot_rotation_dummy'),
    (900904, 110, 110, 7, 131072, 'npc_legionbot_rotation_dummy');

INSERT IGNORE INTO `creature_template_wdb`
    (`Entry`, `Name1`, `Displayid1`, `HpMulti`)
VALUES
    (900901, 'Rotation Dummy: 1 Target', 3019, 1),
    (900902, 'Rotation Dummy: 2 Targets', 3019, 1),
    (900904, 'Rotation Dummy: 4 Targets', 3019, 1);

INSERT IGNORE INTO `creature`
    (`guid`, `id`, `map`, `zoneId`, `areaId`, `spawnMask`, `phaseMask`,
     `position_x`, `position_y`, `position_z`, `orientation`, `spawntimesecs`, `curhealth`)
VALUES
    (900901001, 900901, 1, 876, 876, 1, 1, 16282.0, 16275.0, 17.37, 3.14159, 300, 5000000),
    (900901002, 900902, 1, 876, 876, 1, 1, 16296.0, 16273.0, 18.06, 3.14159, 300, 5000000),
    (900901003, 900902, 1, 876, 876, 1, 1, 16296.0, 16277.0, 16.97, 3.14159, 300, 5000000),
    (900901004, 900904, 1, 876, 876, 1, 1, 16312.0, 16273.0, 17.79, 3.14159, 300, 5000000),
    (900901005, 900904, 1, 876, 876, 1, 1, 16315.0, 16273.0, 17.61, 3.14159, 300, 5000000),
    (900901006, 900904, 1, 876, 876, 1, 1, 16312.0, 16276.0, 16.51, 3.14159, 300, 5000000),
    (900901007, 900904, 1, 876, 876, 1, 1, 16315.0, 16276.0, 16.21, 3.14159, 300, 5000000);
