-- Restore the missing coordinates for Prisoners No More and Threat Reduction.
-- Use the archived objective polygons and points with the 7.3.5 map definitions.
-- Completion points use the local Turalyon in phase 1040.
INSERT INTO `quest_poi_points` (`QuestID`, `Idx1`, `Idx2`, `X`, `Y`, `VerifiedBuild`)
SELECT p.`QuestID`, p.`Idx1`, p.`Idx2`, p.`X`, p.`Y`, p.`VerifiedBuild`
FROM (
    SELECT 46840 AS `QuestID`, 0 AS `Idx1`, 0 AS `Idx2`,
        -1 AS `ObjectiveIndex`, 1304 AS `X`, 1909 AS `Y`, 28153 AS `VerifiedBuild`
    UNION ALL SELECT 46840, 1,  0, -1, 1311, 1909, 28153
    UNION ALL SELECT 46840, 2,  0,  0, 1210, 1630, 28153
    UNION ALL SELECT 46840, 2,  1,  0, 1285, 1573, 28153
    UNION ALL SELECT 46840, 2,  2,  0, 1345, 1565, 28153
    UNION ALL SELECT 46840, 2,  3,  0, 1387, 1583, 28153
    UNION ALL SELECT 46840, 2,  4,  0, 1380, 1631, 28153
    UNION ALL SELECT 46840, 2,  5,  0, 1339, 1676, 28153
    UNION ALL SELECT 46840, 2,  6,  0, 1308, 1745, 28153
    UNION ALL SELECT 46840, 2,  7,  0, 1298, 1831, 28153
    UNION ALL SELECT 46840, 2,  8,  0, 1263, 1898, 28153
    UNION ALL SELECT 46840, 2,  9,  0, 1201, 1903, 28153
    UNION ALL SELECT 46840, 2, 10,  0, 1186, 1799, 28153
    UNION ALL SELECT 46840, 2, 11,  0, 1193, 1709, 28153
    UNION ALL SELECT 46840, 3,  0, 32, 1320, 1902,     0
    UNION ALL SELECT 46841, 0,  0, -1, 1321, 1901, 28153
    UNION ALL SELECT 46841, 1,  0,  0, 1226, 1944, 28153
    UNION ALL SELECT 46841, 2,  0,  1, 1359, 1606, 28153
    UNION ALL SELECT 46841, 3,  0,  2, 1287, 1709, 28153
    UNION ALL SELECT 46841, 4,  0, 32, 1320, 1902,     0
) AS p
JOIN `quest_poi` AS b ON b.`QuestID` = p.`QuestID` AND b.`Idx1` = p.`Idx1`
    AND b.`BlobIndex` = 0 AND b.`ObjectiveIndex` = p.`ObjectiveIndex`
    AND b.`MapID` = 1669 AND b.`WorldMapAreaId` = 1135
    AND b.`Floor` = 0 AND b.`PlayerConditionID` = 0
WHERE NOT EXISTS (
    SELECT 1 FROM `quest_poi_points` AS x WHERE x.`QuestID` = p.`QuestID` AND x.`Idx1` = p.`Idx1`
) AND NOT EXISTS (
    SELECT 1 FROM `quest_poi` AS other
    WHERE other.`QuestID` = p.`QuestID` AND other.`Idx1` = p.`Idx1` AND other.`BlobIndex` <> 0
);
