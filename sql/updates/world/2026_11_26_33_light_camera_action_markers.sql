-- Bind the retained belt markers to each variant's actual delivery objective.
UPDATE `quest_poi` AS p
JOIN `quest_objectives` AS o ON o.`QuestID` = p.`QuestID` AND o.`StorageIndex` = 2 AND o.`ObjectID` = 3593
SET p.`QuestObjectiveID` = o.`ID`
WHERE p.`BlobIndex` = 0 AND p.`ObjectiveIndex` = 2 AND p.`QuestObjectID` = 3593
  AND ((p.`QuestID` = 32470 AND p.`Idx1` = 2 AND p.`QuestObjectiveID` = 295183 AND p.`MapID` = 0 AND p.`WorldMapAreaId` = 301)
    OR (p.`QuestID` = 32471 AND p.`Idx1` = 0 AND p.`QuestObjectiveID` = 295185 AND p.`MapID` = 1 AND p.`WorldMapAreaId` = 321));

-- The Horde variant has just the belt definition. Add its own giver and chest markers.
INSERT IGNORE INTO `quest_poi` (`QuestID`, `BlobIndex`, `Idx1`, `ObjectiveIndex`, `QuestObjectiveID`, `QuestObjectID`,
    `MapID`, `WorldMapAreaId`, `Floor`, `Priority`, `Flags`, `WorldEffectID`, `PlayerConditionID`, `WoDUnk1`,
    `AlwaysAllowMergingBlobs`, `VerifiedBuild`)
SELECT 32471, 0, 1, -1, 0, 0, 1, 321, 0, 0, 0, 0, 0, 0, 0, 0
FROM `creature_questender` WHERE `quest` = 32471 AND `id` = 16926
UNION ALL
SELECT 32471, 0, 2, 0, o.`ID`, o.`ObjectID`, 1, 321, 0, 0, 0, 0, 0, 0, 0, 0
FROM `quest_objectives` AS o WHERE o.`QuestID` = 32471 AND o.`StorageIndex` = 0 AND o.`ObjectID` = 155856;

INSERT INTO `quest_poi_points` (`QuestID`, `Idx1`, `Idx2`, `X`, `Y`, `VerifiedBuild`)
SELECT p.`QuestID`, p.`Idx1`, 0, ROUND(source.`position_x`), ROUND(source.`position_y`), 0
FROM `quest_poi` AS p
JOIN (
    SELECT 32470 AS quest, 0 AS marker, -1 AS objective, c.`map`, c.`position_x`, c.`position_y`
    FROM `creature` AS c JOIN `creature_questender` AS r ON r.`id` = c.`id` AND r.`quest` = 32470
    WHERE c.`guid` = 12889907 AND c.`zoneId` = 1519
    UNION ALL
    SELECT 32471, 1, -1, c.`map`, c.`position_x`, c.`position_y`
    FROM `creature` AS c JOIN `creature_questender` AS r ON r.`id` = c.`id` AND r.`quest` = 32471
    WHERE c.`guid` = 12897439 AND c.`zoneId` = 1637
    UNION ALL
    SELECT IF(g.`map` = 0, 32470, 32471), IF(g.`map` = 0, 1, 2), 0, g.`map`, g.`position_x`, g.`position_y`
    FROM `gameobject` AS g
    JOIN `gameobject_template` AS t ON t.`entry` = g.`id` AND t.`type` = 3
    JOIN `gameobject_loot_template` AS l ON l.`Entry` = t.`Data1` AND l.`Item` = 155856 AND l.`QuestRequired` = 1
    WHERE (g.`guid` = 25283217 AND g.`map` = 0 AND g.`zoneId` = 1519)
       OR (g.`guid` = 25283218 AND g.`map` = 1 AND g.`zoneId` = 1637)
    UNION ALL
    SELECT IF(c.`map` = 0, 32470, 32471), IF(c.`map` = 0, 2, 0), 2, c.`map`, c.`position_x`, c.`position_y`
    FROM `creature` AS c JOIN `npc_vendor` AS v ON v.`entry` = c.`id` AND v.`item` = 3593 AND v.`ExtendedCost` = 0
    WHERE (c.`guid` = 265121 AND c.`map` = 0 AND c.`zoneId` = 1519)
       OR (c.`guid` = 264678 AND c.`map` = 1 AND c.`zoneId` = 1637)
) AS source ON source.quest = p.`QuestID` AND source.marker = p.`Idx1`
  AND source.objective = p.`ObjectiveIndex` AND source.`map` = p.`MapID`
WHERE p.`BlobIndex` = 0 AND p.`Floor` = 0
  AND ((p.`QuestID` = 32470 AND p.`WorldMapAreaId` = 301) OR (p.`QuestID` = 32471 AND p.`WorldMapAreaId` = 321))
  AND NOT EXISTS (SELECT 1 FROM `quest_poi_points` AS point
                  WHERE point.`QuestID` = p.`QuestID` AND point.`Idx1` = p.`Idx1`);
