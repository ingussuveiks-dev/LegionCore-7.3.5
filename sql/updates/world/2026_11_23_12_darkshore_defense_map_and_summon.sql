START TRANSACTION;

-- TDB 7.3.5 retains the position for 65372 effect 0, and TDB 4.3.4 retains its facing.
-- The 26972 spell still uses TARGET_DEST_DB.
INSERT INTO `spell_target_position`
    (`id`, `target_map`, `target_position_x`, `target_position_y`, `target_position_z`, `target_orientation`)
VALUES (65372, 1, 4961.64, 130.561, 70.5063, 0.957487)
ON DUPLICATE KEY UPDATE `id` = VALUES(`id`);

-- The original point sets are shared by their old and corrected POI definitions.
-- Keep every point, the turn-in pin and the newer spawn-tracking definition.
CREATE TEMPORARY TABLE `darkshore_corrected_objective_pois` LIKE `quest_poi`;
INSERT INTO `darkshore_corrected_objective_pois`
SELECT * FROM `quest_poi`
WHERE `QuestID` = 13885 AND `VerifiedBuild` = 26124
  AND (`Idx1`, `BlobIndex`, `ObjectiveIndex`, `QuestObjectiveID`, `QuestObjectID`) IN
      ((1,0,0,267642,34323),(2,0,1,267643,34325),(3,1,1,267643,34325),
       (4,2,1,267643,34325),(5,0,2,267644,34324));

DELETE `legacy` FROM `quest_poi` AS `legacy`
JOIN `darkshore_corrected_objective_pois` AS `current`
  ON `current`.`QuestID` = `legacy`.`QuestID` AND `current`.`Idx1` = `legacy`.`Idx1`
 AND `current`.`QuestObjectiveID` = `legacy`.`QuestObjectiveID`
 AND `current`.`QuestObjectID` = `legacy`.`QuestObjectID`
 AND `current`.`MapID` = `legacy`.`MapID`
 AND `current`.`WorldMapAreaId` = `legacy`.`WorldMapAreaId`
 AND `current`.`Floor` = `legacy`.`Floor`
 AND `current`.`Priority` = `legacy`.`Priority`
 AND `current`.`Flags` = `legacy`.`Flags`
 AND `current`.`WorldEffectID` = `legacy`.`WorldEffectID`
 AND `current`.`PlayerConditionID` = `legacy`.`PlayerConditionID`
 AND `current`.`WoDUnk1` = `legacy`.`WoDUnk1`
 AND `current`.`AlwaysAllowMergingBlobs` = `legacy`.`AlwaysAllowMergingBlobs`
WHERE `legacy`.`VerifiedBuild` = 23877 AND `legacy`.`ObjectiveIndex` = 0
  AND (`legacy`.`Idx1`, `legacy`.`BlobIndex`) IN ((1,1),(2,6),(3,5),(4,4),(5,2));

DROP TEMPORARY TABLE `darkshore_corrected_objective_pois`;
COMMIT;
