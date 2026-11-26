START TRANSACTION;

-- The same five Redridge groups appear in Legion builds 22522, 22908 and 24330.
INSERT INTO `quest_poi_points` (`QuestID`,`Idx1`,`Idx2`,`X`,`Y`,`VerifiedBuild`)
SELECT p.`QuestID`,p.`Idx1`,0,s.`X`,s.`Y`,24330
FROM `quest_poi` AS p
JOIN (
    SELECT 1 AS `BlobIndex`,2 AS `Idx1`,0 AS `ObjectiveIndex`,252598 AS `QuestObjectiveID`,341 AS `QuestObjectID`,430276 AS `SpawnTrackingID`,-9113 AS `X`,-2064 AS `Y`
    UNION ALL SELECT 2,3,0,252598,341,430277,-9112,-2353
    UNION ALL SELECT 0,4,32,0,0,431539,-8833,-1937
) AS s ON p.`BlobIndex`=s.`BlobIndex` AND p.`Idx1`=s.`Idx1` AND p.`ObjectiveIndex`=s.`ObjectiveIndex`
    AND p.`QuestObjectiveID`=s.`QuestObjectiveID` AND p.`QuestObjectID`=s.`QuestObjectID` AND p.`WoDUnk1`=s.`SpawnTrackingID`
WHERE p.`QuestID`=26520 AND p.`MapID`=0 AND p.`WorldMapAreaId`=36 AND p.`Floor`=0
    AND p.`Flags`=0 AND p.`WorldEffectID`=0 AND p.`PlayerConditionID`=0
    AND NOT EXISTS (SELECT 1 FROM `quest_poi_points` AS pt WHERE pt.`QuestID`=p.`QuestID` AND pt.`Idx1`=p.`Idx1`);

-- The obsolete rows share the live point indexes. The loader would send a
-- second rescue and a return marker at a different actor's coordinate.
DELETE FROM `quest_poi`
WHERE `QuestID`=26520 AND `MapID`=0 AND `WorldMapAreaId`=36 AND `Floor`=0
    AND `WoDUnk1`=0 AND `VerifiedBuild`=21154
    AND (`BlobIndex`,`Idx1`,`ObjectiveIndex`,`QuestObjectiveID`,`QuestObjectID`,`Flags`) IN
        ((1,1,0,252598,341,1),(2,2,-1,0,0,3));

COMMIT;
