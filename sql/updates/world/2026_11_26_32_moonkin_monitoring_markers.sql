-- Keep each Moonglade objective on the original observer, and the return point on Makkaw.
INSERT INTO `quest_poi_points` (`QuestID`, `Idx1`, `Idx2`, `X`, `Y`, `VerifiedBuild`)
SELECT p.`QuestID`, p.`Idx1`, 0, ROUND(c.`position_x`), ROUND(c.`position_y`), 0
FROM `quest_poi` AS p
JOIN `creature` AS c
  ON c.`guid` IN (146703497, 146703557, 146703559, 146703560)
 AND c.`map` = p.`MapID` AND c.`zoneId` = 493
JOIN `game_event_creature` AS e ON e.`guid` = c.`guid` AND e.`eventEntry` = 299
LEFT JOIN `creature_questender` AS r ON r.`quest` = p.`QuestID` AND r.`id` = c.`id`
LEFT JOIN `quest_objectives` AS o
  ON o.`QuestID` = p.`QuestID` AND o.`ID` = p.`QuestObjectiveID` AND o.`ObjectID` = c.`id`
WHERE p.`QuestID` = 47430 AND p.`BlobIndex` = 0 AND p.`MapID` = 1 AND p.`WorldMapAreaId` = 241 AND p.`Floor` = 0
  AND ((p.`Idx1` = 0 AND p.`ObjectiveIndex` = -1 AND c.`id` = 122134 AND r.`id` IS NOT NULL)
    OR (p.`Idx1` IN (1, 2, 3) AND p.`ObjectiveIndex` BETWEEN 0 AND 2 AND p.`QuestObjectID` = c.`id` AND o.`ID` IS NOT NULL))
  AND NOT EXISTS (SELECT 1 FROM `quest_poi_points` AS point
                  WHERE point.`QuestID` = p.`QuestID` AND point.`Idx1` = p.`Idx1`);
