-- Full normal/heroic/mythic raid records use the beginning of the first LFR
-- wing on the same map. Later-wing LFR positions remain separate and intact.
INSERT IGNORE INTO `lfg_entrances`
    (`dungeonId`, `name`, `position_x`, `position_y`, `position_z`, `orientation`)
SELECT
    mapping.`targetId`,
    mapping.`targetName`,
    source.`position_x`,
    source.`position_y`,
    source.`position_z`,
    source.`orientation`
FROM
(
    SELECT 895 AS `targetId`, 849 AS `sourceId`, 'Highmaul (Normal)' AS `targetName` UNION ALL
    SELECT 896, 849, 'Highmaul (Heroic)' UNION ALL
    SELECT 897, 849, 'Highmaul (Mythic)' UNION ALL
    SELECT 987, 982, 'Hellfire Citadel (Normal)' UNION ALL
    SELECT 988, 982, 'Hellfire Citadel (Heroic)' UNION ALL
    SELECT 989, 982, 'Hellfire Citadel (Mythic)' UNION ALL
    SELECT 1348, 1287, 'The Emerald Nightmare (Normal)' UNION ALL
    SELECT 1349, 1287, 'The Emerald Nightmare (Heroic)' UNION ALL
    SELECT 1350, 1287, 'The Emerald Nightmare (Mythic)' UNION ALL
    SELECT 1351, 1290, 'The Nighthold (Normal)' UNION ALL
    SELECT 1352, 1290, 'The Nighthold (Heroic)' UNION ALL
    SELECT 1353, 1290, 'The Nighthold (Mythic)' UNION ALL
    SELECT 1437, 1411, 'Trial of Valor (Normal)' UNION ALL
    SELECT 1438, 1411, 'Trial of Valor (Mythic)' UNION ALL
    SELECT 1439, 1411, 'Trial of Valor (Heroic)' UNION ALL
    SELECT 1525, 1494, 'Tomb of Sargeras (Normal)' UNION ALL
    SELECT 1526, 1494, 'Tomb of Sargeras (Heroic)' UNION ALL
    SELECT 1527, 1494, 'Tomb of Sargeras (Mythic)' UNION ALL
    SELECT 1640, 1610, 'Antorus, the Burning Throne (Normal)' UNION ALL
    SELECT 1641, 1610, 'Antorus, the Burning Throne (Heroic)' UNION ALL
    SELECT 1642, 1610, 'Antorus, the Burning Throne (Mythic)'
) AS mapping
INNER JOIN `lfg_entrances` AS source
    ON source.`dungeonId` = mapping.`sourceId`;
