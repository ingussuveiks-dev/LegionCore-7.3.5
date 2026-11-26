-- Both captures of the focus marker reference point set 1 (-13, -381) in the Deadmines.
UPDATE `quest_poi`
SET `MapID` = 36, `WorldMapAreaId` = 756, `Floor` = 1, `ObjectiveIndex` = 0
WHERE `QuestID` = 26320 AND `BlobIndex` IN (0, 1) AND `Idx1` = 1 AND `MapID` = 0
  AND `WorldMapAreaId` = 39 AND `Floor` = 0 AND `ObjectiveIndex` = 30
  AND `QuestObjectiveID` = 0 AND `QuestObjectID` = 0 AND `Flags` = 7;
