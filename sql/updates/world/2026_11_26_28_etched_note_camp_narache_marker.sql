-- Resolve the missing marker from the actual Camp Narache receiver, Lanka Farshot.
UPDATE `quest_poi` AS p
JOIN `creature_questender` AS r ON r.`quest`=p.`QuestID` AND r.`id`=3061
JOIN `creature` AS c ON c.`id`=r.`id` AND c.`guid`=169364 AND c.`map`=1 AND c.`zoneId`=6452 AND c.`areaId`=221
SET p.`MapID`=c.`map`
WHERE p.`QuestID`=3092 AND p.`BlobIndex`=0 AND p.`Idx1`=0 AND p.`MapID`=0
  AND p.`WorldMapAreaId`=890 AND p.`ObjectiveIndex`=-1 AND p.`QuestObjectiveID`=256641
  AND NOT EXISTS (SELECT 1 FROM `quest_poi_points` AS pt WHERE pt.`QuestID`=p.`QuestID` AND pt.`Idx1`=p.`Idx1`);

INSERT INTO `quest_poi_points` (`QuestID`,`Idx1`,`Idx2`,`X`,`Y`,`VerifiedBuild`)
SELECT p.`QuestID`,p.`Idx1`,0,ROUND(c.`position_x`),ROUND(c.`position_y`),0
FROM `quest_poi` AS p
JOIN `creature_questender` AS r ON r.`quest`=p.`QuestID` AND r.`id`=3061
JOIN `creature` AS c ON c.`id`=r.`id` AND c.`guid`=169364 AND c.`map`=1 AND c.`zoneId`=6452 AND c.`areaId`=221
WHERE p.`QuestID`=3092 AND p.`BlobIndex`=0 AND p.`Idx1`=0 AND p.`MapID`=c.`map`
  AND p.`WorldMapAreaId`=890 AND p.`ObjectiveIndex`=-1 AND p.`QuestObjectiveID`=256641
  AND NOT EXISTS (SELECT 1 FROM `quest_poi_points` AS pt WHERE pt.`QuestID`=p.`QuestID` AND pt.`Idx1`=p.`Idx1`);
