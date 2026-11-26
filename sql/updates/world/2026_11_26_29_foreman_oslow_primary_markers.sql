-- Restore the return and rescue points from the actual Redridge actors.
INSERT INTO `quest_poi_points` (`QuestID`,`Idx1`,`Idx2`,`X`,`Y`,`VerifiedBuild`)
SELECT p.`QuestID`,p.`Idx1`,0,ROUND(c.`position_x`),ROUND(c.`position_y`),0
FROM `quest_poi` AS p
JOIN `creature_questender` AS r ON r.`quest`=p.`QuestID` AND r.`id`=344
JOIN `creature` AS c ON c.`id`=r.`id` AND c.`guid`=38626 AND c.`map`=0 AND c.`zoneId`=44 AND c.`areaId`=69
WHERE p.`QuestID`=26520 AND p.`BlobIndex`=0 AND p.`Idx1`=0 AND p.`ObjectiveIndex`=-1
  AND p.`MapID`=c.`map` AND p.`WorldMapAreaId`=36
  AND NOT EXISTS (SELECT 1 FROM `quest_poi_points` AS pt WHERE pt.`QuestID`=p.`QuestID` AND pt.`Idx1`=p.`Idx1`);

INSERT INTO `quest_poi_points` (`QuestID`,`Idx1`,`Idx2`,`X`,`Y`,`VerifiedBuild`)
SELECT p.`QuestID`,p.`Idx1`,0,ROUND(c.`position_x`),ROUND(c.`position_y`),0
FROM `quest_poi` AS p
JOIN `quest_objectives` AS o ON o.`QuestID`=p.`QuestID` AND o.`ID`=p.`QuestObjectiveID` AND o.`ObjectID`=341
JOIN `creature` AS c ON c.`id`=o.`ObjectID` AND c.`guid`=38729 AND c.`map`=0 AND c.`zoneId`=44 AND c.`areaId`=69
WHERE p.`QuestID`=26520 AND p.`BlobIndex`=0 AND p.`Idx1`=1 AND p.`ObjectiveIndex`=0
  AND p.`QuestObjectiveID`=252598 AND p.`QuestObjectID`=c.`id` AND p.`MapID`=c.`map` AND p.`WorldMapAreaId`=36
  AND NOT EXISTS (SELECT 1 FROM `quest_poi_points` AS pt WHERE pt.`QuestID`=p.`QuestID` AND pt.`Idx1`=p.`Idx1`);
