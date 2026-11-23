-- Restore Legion 7.3.5 map coordinates for the Twins artifact and Mage Tower route.
-- Matching TDB 735.00 points retain build 24330. The local Karlain spawn and copied
-- Investigating the Legion map variant use build 0.
START TRANSACTION;

-- Idx1 is the coordinate key even when BlobIndex differs. Move the gated
-- Investigating the Legion search area off the starter's occupied index.
UPDATE `quest_poi` AS `p`
LEFT JOIN `quest_poi` AS `reserved`
  ON `reserved`.`QuestID` = `p`.`QuestID` AND `reserved`.`Idx1` = 4
LEFT JOIN `quest_poi_points` AS `existing`
  ON `existing`.`QuestID` = `p`.`QuestID` AND `existing`.`Idx1` IN (2, 4)
SET `p`.`Idx1` = 4
WHERE `p`.`QuestID` = 46765 AND `p`.`BlobIndex` = 1 AND `p`.`Idx1` = 2
  AND `p`.`ObjectiveIndex` = 0 AND `p`.`QuestObjectiveID` = 289571
  AND `p`.`QuestObjectID` = 147430 AND `p`.`MapID` = 1220
  AND `p`.`WorldMapAreaId` = 1021 AND `p`.`PlayerConditionID` = 48140
  AND `reserved`.`QuestID` IS NULL AND `existing`.`QuestID` IS NULL;

CREATE TEMPORARY TABLE `twins_restored_poi_points` LIKE `quest_poi_points`;

INSERT INTO `twins_restored_poi_points` (`QuestID`, `Idx1`, `Idx2`, `X`, `Y`, `VerifiedBuild`) VALUES
    (45182, 0, 0, -802, 4467, 24330),
    (45182, 1, 0, -845, 4638, 0),
    (45185, 0, 0, -802, 4467, 24330),
    (45185, 1, 0, -797, 4486, 24330),
    (45185, 2, 0, -802, 4467, 24330),
    (45187, 0, 0, -852, 4455, 24330),
    (45187, 1, 0, -918, 4378, 24330),
    (45187, 1, 1, -812, 4402, 24330),
    (45187, 1, 2, -789, 4418, 24330),
    (45187, 1, 3, -769, 4459, 24330),
    (45187, 1, 4, -766, 4524, 24330),
    (45187, 1, 5, -772, 4545, 24330),
    (45187, 1, 6, -823, 4538, 24330),
    (45187, 1, 7, -841, 4529, 24330),
    (45187, 1, 8, -872, 4491, 24330),
    (45187, 1, 9, -891, 4462, 24330),
    (45187, 1, 10, -899, 4444, 24330),
    (45187, 1, 11, -919, 4394, 24330),
    (45187, 2, 0, -845, 4379, 24330),
    (45187, 3, 0, -802, 4467, 24330),
    (45188, 0, 0, -845, 4638, 0),
    (45188, 1, 0, -853, 4456, 24330),
    (45190, 0, 0, -706, 2743, 24330),
    (45190, 1, 0, -706, 2743, 24330),
    (45190, 2, 0, -845, 4638, 0),
    (45190, 3, 0, -845, 4638, 24330),
    (45190, 4, 0, -739, 4522, 24330),
    (45192, 0, 0, -708, 2615, 24330),
    (45192, 1, 0, -704, 2627, 24330),
    (45192, 2, 0, -706, 2743, 24330),
    (45193, 0, 0, -848, 4639, 24330),
    (45193, 1, 0, -708, 2615, 24330),
    (45866, 0, 0, -852, 4639, 24330),
    (45866, 1, 0, -851, 4625, 24330),
    (45866, 2, 0, -852, 4639, 24330),
    (45910, 0, 0, -845, 4638, 0),
    (45910, 1, 0, -851, 4627, 24330),
    (45910, 2, 0, -851, 4630, 24330),
    (46127, 0, 0, -845, 4638, 0),
    (46127, 1, 0, -1597, 3156, 24330),
    (46127, 2, 0, -1597, 3157, 24330),
    (46765, 0, 0, -1631, 3199, 24330),
    (46765, 1, 0, -802, 1502, 24330),
    (46765, 1, 1, -329, 1861, 24330),
    (46765, 1, 2, -140, 2551, 24330),
    (46765, 1, 3, -175, 3035, 24330),
    (46765, 1, 4, -500, 3622, 24330),
    (46765, 1, 5, -944, 3913, 24330),
    (46765, 1, 6, -1537, 4007, 24330),
    (46765, 1, 7, -2007, 3449, 24330),
    (46765, 1, 8, -2078, 3064, 24330),
    (46765, 1, 9, -2007, 2591, 24330),
    (46765, 1, 10, -1788, 2072, 24330),
    (46765, 1, 11, -1528, 1676, 24330),
    (46765, 2, 0, -1631, 3199, 24330),
    (46765, 3, 0, -1631, 3199, 0),
    (46765, 4, 0, -802, 1502, 0),
    (46765, 4, 1, -329, 1861, 0),
    (46765, 4, 2, -140, 2551, 0),
    (46765, 4, 3, -175, 3035, 0),
    (46765, 4, 4, -500, 3622, 0),
    (46765, 4, 5, -944, 3913, 0),
    (46765, 4, 6, -1537, 4007, 0),
    (46765, 4, 7, -2007, 3449, 0),
    (46765, 4, 8, -2078, 3064, 0),
    (46765, 4, 9, -2007, 2591, 0),
    (46765, 4, 10, -1788, 2072, 0),
    (46765, 4, 11, -1528, 1676, 0),
    (47034, 0, 0, -845, 4638, 0),
    (47034, 1, 0, -1631, 3199, 24330),
    (47037, 0, 0, -802, 4467, 24330),
    (47037, 1, 0, -845, 4638, 0),
    (47039, 0, 0, -802, 4467, 24330),
    (47039, 1, 0, -845, 4638, 0),
    (47041, 0, 0, -802, 4467, 24330),
    (47041, 1, 0, -845, 4638, 0),
    (47042, 0, 0, -802, 4467, 24330),
    (47042, 1, 0, -845, 4638, 0);

-- Restore complete missing point sets. Leave any existing point set intact.
INSERT INTO `quest_poi_points` (`QuestID`, `Idx1`, `Idx2`, `X`, `Y`, `VerifiedBuild`)
SELECT `s`.`QuestID`, `s`.`Idx1`, `s`.`Idx2`, `s`.`X`, `s`.`Y`, `s`.`VerifiedBuild`
FROM `twins_restored_poi_points` AS `s`
JOIN `quest_poi` AS `p`
  ON `p`.`QuestID` = `s`.`QuestID` AND `p`.`Idx1` = `s`.`Idx1`
 AND `p`.`BlobIndex` = CASE
      WHEN `s`.`QuestID` = 46765 AND `s`.`Idx1` = 4 THEN 1
      WHEN `s`.`QuestID` = 45190 AND `s`.`Idx1` IN (1, 3) THEN 1
      WHEN `s`.`QuestID` = 45190 AND `s`.`Idx1` = 4 THEN 2
      ELSE 0
 END
LEFT JOIN `quest_poi_points` AS `existing`
  ON `existing`.`QuestID` = `s`.`QuestID` AND `existing`.`Idx1` = `s`.`Idx1`
WHERE `existing`.`QuestID` IS NULL
  AND (`s`.`QuestID` <> 46765 OR `s`.`Idx1` <> 2 OR NOT EXISTS (
      SELECT 1 FROM `quest_poi` AS `alternate`
      WHERE `alternate`.`QuestID` = 46765 AND `alternate`.`Idx1` = 2 AND `alternate`.`BlobIndex` <> 0
  ));

DROP TEMPORARY TABLE `twins_restored_poi_points`;

COMMIT;
