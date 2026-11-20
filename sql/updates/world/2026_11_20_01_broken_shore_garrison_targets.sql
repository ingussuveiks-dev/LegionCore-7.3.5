-- Break Their Garrison (46073). Locally authored placements on checked 26972 terrain.
-- Use the existing Garrison phase 7147 that excludes the Felskorn variants.
-- GUIDs are reserved for this migration. Replay preserves moved rows and rejects an unrelated GUID collision.
START TRANSACTION;

INSERT INTO `creature`
    (`guid`, `id`, `map`, `zoneId`, `areaId`, `spawnMask`, `phaseMask`, `PhaseId`,
     `position_x`, `position_y`, `position_z`, `orientation`, `spawntimesecs`)
VALUES
    (146980460, 117555, 1220, 7543, 8551, 1, 1, '7147', -1253, 2704, 27.065, 4.50, 45),
    (146980461, 117555, 1220, 7543, 8551, 1, 1, '7147', -1263, 2704, 26.657, 5.85, 45),
    (146980462, 117555, 1220, 7543, 8551, 1, 1, '7147', -1263, 2694, 26.413, 0.65, 45),
    (146980463, 117555, 1220, 7543, 8551, 1, 1, '7147', -1236, 2675, 26.942, 1.10, 45),
    (146980464, 117555, 1220, 7543, 8551, 1, 1, '7147', -1231, 2680, 27.506, 3.95, 45),
    (146980465, 117555, 1220, 7543, 8551, 1, 1, '7147', -1231, 2670, 27.230, 2.20, 45),
    (146980466, 117555, 1220, 7543, 8551, 1, 1, '7147', -1203, 2666, 29.629, 3.65, 45),
    (146980467, 117555, 1220, 7543, 8551, 1, 1, '7147', -1212, 2670, 28.881, 5.60, 45),
    (146980468, 117555, 1220, 7543, 8551, 1, 1, '7147', -1206, 2661, 29.690, 1.85, 45),
    (146980469, 117555, 1220, 7543, 8551, 1, 1, '7147', -1189, 2639, 28.634, 0.30, 45),
    (146980470, 117555, 1220, 7543, 8551, 1, 1, '7147', -1184, 2644, 28.431, 4.45, 45),
    (146980471, 117555, 1220, 7543, 8551, 1, 1, '7147', -1184, 2634, 28.299, 2.45, 45),
    (146980472, 118720, 1220, 7543, 8551, 1, 1, '7147', -1159, 2638, 27.585, 3.10, 240)
ON DUPLICATE KEY UPDATE
    `id` = IF(`creature`.`id` = VALUES(`id`) AND `creature`.`map` = VALUES(`map`), `creature`.`id`, NULL);

INSERT INTO `gameobject`
    (`guid`, `id`, `map`, `zoneId`, `areaId`, `spawnMask`, `phaseMask`, `PhaseId`,
     `position_x`, `position_y`, `position_z`, `orientation`, `rotation0`, `rotation1`,
     `rotation2`, `rotation3`, `spawntimesecs`, `animprogress`, `state`)
VALUES
    (25760460, 269154, 1220, 7543, 8551, 1, 1, '7147', -1290, 2639, 32.610, 1.5707963, 0, 0, 0.7071068, 0.7071068, 120, 255, 1),
    (25760461, 269154, 1220, 7543, 8551, 1, 1, '7147', -1237, 2694, 27.811, 0.0000000, 0, 0, 0.0000000, 1.0000000, 120, 255, 1),
    (25760462, 269154, 1220, 7543, 8551, 1, 1, '7147', -1209, 2652, 29.392, 4.7123890, 0, 0, 0.7071068,-0.7071068, 120, 255, 1),
    (25760463, 269154, 1220, 7543, 8551, 1, 1, '7147', -1164, 2625, 28.105, 3.1415927, 0, 0, 1.0000000, 0.0000000, 120, 255, 1)
ON DUPLICATE KEY UPDATE
    `id` = IF(`gameobject`.`id` = VALUES(`id`) AND `gameobject`.`map` = VALUES(`map`), `gameobject`.`id`, NULL);

-- Native goober admission guards spell 241545 and GAMEOBJECT objective 289938.
UPDATE `gameobject_template`
SET `Data1` = 46073,
    `name` = IF(`name` = 'The knowledge manuscript of Badgers.', 'Libram of Fel Knowledge', `name`)
WHERE `entry` = 269154 AND `type` = 10 AND `displayId` = 26998 AND `Data10` = 241545 AND `Data1` = 0;

COMMIT;
