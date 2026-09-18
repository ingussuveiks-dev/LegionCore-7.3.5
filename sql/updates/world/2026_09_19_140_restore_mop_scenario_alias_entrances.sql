-- Build 26972 contains later aliases for the Pandaria scenario records. Their
-- map, scenario and difficulty values match the older records already backed
-- by verified entrance data, so reuse those exact positions for the aliases.
INSERT IGNORE INTO `lfg_entrances`
    (`dungeonId`, `name`, `position_x`, `position_y`, `position_z`, `orientation`)
SELECT
    mapping.`targetId`,
    CONCAT(source.`name`, ' (build 26972 alias)'),
    source.`position_x`,
    source.`position_y`,
    source.`position_z`,
    source.`orientation`
FROM
(
    SELECT 790 AS `targetId`, 492 AS `sourceId` UNION ALL
    SELECT 791, 499 UNION ALL
    SELECT 792, 504 UNION ALL
    SELECT 793, 511 UNION ALL
    SELECT 794, 517 UNION ALL
    SELECT 795, 537 UNION ALL
    SELECT 796, 539 UNION ALL
    SELECT 798, 542 UNION ALL
    SELECT 799, 586 UNION ALL
    SELECT 800, 588 UNION ALL
    SELECT 801, 589 UNION ALL
    SELECT 803, 593 UNION ALL
    SELECT 804, 595 UNION ALL
    SELECT 805, 586 UNION ALL
    SELECT 806, 589 UNION ALL
    SELECT 807, 624 UNION ALL
    SELECT 808, 625 UNION ALL
    SELECT 809, 637 UNION ALL
    SELECT 810, 639 UNION ALL
    SELECT 811, 646 UNION ALL
    SELECT 812, 647 UNION ALL
    SELECT 813, 648 UNION ALL
    SELECT 814, 649 UNION ALL
    SELECT 815, 652 UNION ALL
    SELECT 816, 654 UNION ALL
    SELECT 817, 655
) AS mapping
INNER JOIN `lfg_entrances` AS source
    ON source.`dungeonId` = mapping.`sourceId`;
