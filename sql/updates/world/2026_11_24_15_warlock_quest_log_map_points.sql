-- Restore map guidance for eight Legion quest-log entries.
-- Idx1 owns the coordinates within a quest, independently of BlobIndex.
START TRANSACTION;

CREATE TEMPORARY TABLE `quest_log_map_positions` (
    `QuestID` INT NOT NULL,
    `BlobIndex` INT NOT NULL,
    `Idx1` INT NOT NULL,
    `ObjectiveIndex` INT NOT NULL,
    `QuestObjectiveID` INT NOT NULL,
    `QuestObjectID` INT NOT NULL,
    `MapID` INT NOT NULL,
    `WorldMapAreaId` INT NOT NULL,
    `Floor` INT NOT NULL,
    `PlayerConditionID` INT NOT NULL,
    `X` INT NOT NULL,
    `Y` INT NOT NULL,
    `VerifiedBuild` INT NOT NULL,
    `AddDefinition` TINYINT NOT NULL,
    PRIMARY KEY (`QuestID`, `Idx1`)
) ENGINE=InnoDB;

INSERT INTO `quest_log_map_positions` VALUES
    -- Pillars of Creation and The Deceiver's Downfall share the outdoor raid access point.
    -- The first Pillars objective anchors the shared raid marker.
    (46253, 0, 0,  0, 289283, 120408, 1220, 1021,  0,     0,  -553,  2453,     0, 1),
    (46253, 0, 1, 32,      0,      0, 1220, 1021,  0,     0, -1628,  3194,     0, 1),
    (46805, 0, 0,  0, 289632,  58097, 1220, 1021,  0,     0,  -553,  2453,     0, 1),
    (46805, 0, 1, 32,      0,      0, 1220, 1021,  0,     0, -1631,  3201,     0, 1),

    -- TDB 735.00 coordinates for Khadgar, the three books and Maiev.
    (46744, 0, 0, -1,      0,      0, 1220, 1021,  0,     0, -1628,  3194, 24330, 0),
    (46744, 0, 1, 32,      0,      0, 1220, 1021,  0,     0, -1628,  3194, 24330, 0),
    (47067, 0, 0, -1,      0,      0, 1220, 1021,  0,     0, -1628,  3194, 24330, 0),
    (47067, 0, 1,  0, 289939, 121158, 1220, 1021,  0,     0, -1162,  1738, 24330, 0),
    (47067, 0, 2,  1, 289940, 121164, 1220, 1021,  0,     0, -1049,  2147, 24330, 0),
    (47067, 0, 3,  2, 289941, 121165, 1220, 1021,  0,     0,  -935,  2005, 24330, 0),
    (47067, 0, 4, 32,      0,      0, 1220, 1021,  0,     0, -1628,  3194, 24330, 0),
    (47137, 0, 0, -1,      0,      0, 1220, 1021,  0,     0, -1635,  3202, 24330, 0),
    (47137, 0, 1, 32,      0,      0, 1220, 1021,  0,     0, -1635,  3202, 24330, 0),

    -- The Hand of Fate (Horde). Keep both Dalaran floor and Bladefist Bay variants.
    (47835, 0, 0, -1,      0,      0,    1,    4,  0,     0,  1388, -5033,     0, 0),
    (47835, 0, 1,  0, 290999, 124365, 1220, 1014, 10, 51727,  -713,  4420,     0, 0),
    (47835, 1, 2,  0, 290999, 124365, 1220, 1014, 10, 51726,  -789,  4421,     0, 0),
    (47835, 0, 3,  1, 291000, 121751,    1,    4,  0,     0,  1384, -5037,     0, 0),
    (47835, 1, 4,  1, 291000, 121751,    1,  321,  0, 53137,  1380, -5041,     0, 0),

    -- Jaelaan's three Vindicaar locations share their respective floor conditions.
    -- The separate Argus world quest markers supply the rotating objective locations.
    (48639, 0, 0, -1,      0,      0, 1669, 1135,  1, 52311,   404,  1470,     0, 0),
    (48639, 1, 1, -1,      0,      0, 1669, 1170,  3, 51972,  4632,  9883,     0, 0),
    (48639, 2, 2, -1,      0,      0, 1669, 1171,  5, 51973, -2654,  8604,     0, 0),
    (48639, 0, 3, 32,      0,      0, 1669, 1135,  1, 52311,   404,  1470,     0, 1),
    (48639, 1, 4, 32,      0,      0, 1669, 1170,  3, 51972,  4632,  9883,     0, 1),
    (48639, 2, 5, 32,      0,      0, 1669, 1171,  5, 51973, -2654,  8604,     0, 1),

    -- Sylvanas in Orgrimmar is both the start and turn-in for The Call for Allies.
    (49930, 0, 0, -1,      0,      0,    1,  321,  0,     0,  1546, -4163,     0, 0),
    (49930, 0, 1, 32,      0,      0,    1,  321,  0,     0,  1546, -4163,     0, 1);

-- Leave a locally populated index intact, including an orphaned coordinate set.
INSERT INTO `quest_poi` (`QuestID`, `BlobIndex`, `Idx1`, `ObjectiveIndex`, `QuestObjectiveID`, `QuestObjectID`,
    `MapID`, `WorldMapAreaId`, `Floor`, `Priority`, `Flags`, `WorldEffectID`, `PlayerConditionID`, `WoDUnk1`,
    `AlwaysAllowMergingBlobs`, `VerifiedBuild`)
SELECT p.`QuestID`, p.`BlobIndex`, p.`Idx1`, p.`ObjectiveIndex`, p.`QuestObjectiveID`, p.`QuestObjectID`,
    p.`MapID`, p.`WorldMapAreaId`, p.`Floor`, 0, 0, 0, p.`PlayerConditionID`, 0, 0, p.`VerifiedBuild`
FROM `quest_log_map_positions` AS p
WHERE p.`AddDefinition` = 1
    AND NOT EXISTS (SELECT 1 FROM `quest_poi` AS b WHERE b.`QuestID` = p.`QuestID` AND b.`Idx1` = p.`Idx1`)
    AND NOT EXISTS (SELECT 1 FROM `quest_poi_points` AS x WHERE x.`QuestID` = p.`QuestID` AND x.`Idx1` = p.`Idx1`);

INSERT INTO `quest_poi_points` (`QuestID`, `Idx1`, `Idx2`, `X`, `Y`, `VerifiedBuild`)
SELECT p.`QuestID`, p.`Idx1`, 0, p.`X`, p.`Y`, p.`VerifiedBuild`
FROM `quest_log_map_positions` AS p
JOIN `quest_poi` AS b ON b.`QuestID` = p.`QuestID` AND b.`Idx1` = p.`Idx1`
    AND b.`BlobIndex` = p.`BlobIndex` AND b.`ObjectiveIndex` = p.`ObjectiveIndex`
    AND b.`QuestObjectiveID` = p.`QuestObjectiveID` AND b.`QuestObjectID` = p.`QuestObjectID`
    AND b.`MapID` = p.`MapID` AND b.`WorldMapAreaId` = p.`WorldMapAreaId`
    AND b.`Floor` = p.`Floor` AND b.`PlayerConditionID` = p.`PlayerConditionID`
WHERE NOT EXISTS (SELECT 1 FROM `quest_poi_points` AS x WHERE x.`QuestID` = p.`QuestID` AND x.`Idx1` = p.`Idx1`)
    AND NOT EXISTS (SELECT 1 FROM `quest_poi` AS other WHERE other.`QuestID` = p.`QuestID`
        AND other.`Idx1` = p.`Idx1` AND other.`BlobIndex` <> p.`BlobIndex`);

DROP TEMPORARY TABLE `quest_log_map_positions`;
COMMIT;
