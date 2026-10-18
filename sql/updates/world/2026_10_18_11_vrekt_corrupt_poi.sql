-- Remove only the malformed imported row. Its huge objective index,
-- negative objective/object/map IDs and unrelated world-map area are not
-- native Vrekt data. Keep both valid counter polygons and all their points,
-- Vaettfang's pin and the quest tracking pin, including the other Idx1=1 row.
DELETE FROM `quest_poi`
WHERE `QuestID`=44923 AND `BlobIndex`=0 AND `Idx1`=1
  AND `ObjectiveIndex`=1191182336 AND `QuestObjectiveID`=-2063596446
  AND `QuestObjectID`=-1006632508 AND `MapID`=-117440508
  AND `WorldMapAreaId`=3;
