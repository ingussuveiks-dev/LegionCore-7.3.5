-- Quest 44737 uses NPC 115739 as a virtual 5% progress credit for destroying
-- Kvaldir supplies. Reuse Legion-era interactive weapon rack and banner models.
UPDATE `gameobject_template`
SET `ScriptName` = 'go_grey_shoals_supplies'
WHERE `entry` IN (241741, 244704) AND `ScriptName` = '';

INSERT INTO `gameobject`
    (`guid`, `id`, `map`, `zoneId`, `areaId`, `spawnMask`, `phaseMask`,
     `position_x`, `position_y`, `position_z`, `orientation`, `rotation3`,
     `spawntimesecs`, `animprogress`, `state`)
VALUES
    (147040012, 241741, 1220, 7334, 8297, 1, 1, 465.0, 7778.0, 2.1, 0, 1, 180, 255, 1),
    (147040013, 244704, 1220, 7334, 8297, 1, 1, 530.0, 7735.0, 0.7, 0, 1, 180, 255, 1),
    (147040014, 241741, 1220, 7334, 8297, 1, 1, 559.0, 7699.0, 2.2, 0, 1, 180, 255, 1),
    (147040015, 244704, 1220, 7334, 8297, 1, 1, 571.0, 7654.0, 3.5, 0, 1, 180, 255, 1)
ON DUPLICATE KEY UPDATE
    `id` = VALUES(`id`),
    `map` = VALUES(`map`),
    `zoneId` = VALUES(`zoneId`),
    `areaId` = VALUES(`areaId`),
    `position_x` = VALUES(`position_x`),
    `position_y` = VALUES(`position_y`),
    `position_z` = VALUES(`position_z`),
    `spawntimesecs` = VALUES(`spawntimesecs`);
