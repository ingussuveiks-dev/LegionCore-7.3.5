-- Reuse the established entrance of the same instance/wing for two build
-- 26972 variants that differ only by difficulty or alias ID.
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
    SELECT 1005 AS `targetId`, 787 AS `sourceId`, 'Bloodmaul Slag Mines (Mythic)' AS `targetName` UNION ALL
    SELECT 1366, 982, 'Hellbreach (build 26972 alias)'
) AS mapping
INNER JOIN `lfg_entrances` AS source
    ON source.`dungeonId` = mapping.`sourceId`;
