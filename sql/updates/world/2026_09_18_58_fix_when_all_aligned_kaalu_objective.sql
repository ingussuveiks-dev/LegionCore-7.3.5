-- Quest 35704 uses the spawned, scripted Ka'alu entry 77857. Its SmartAI
-- awards kill credit 77857; entry 82651 has no spawn or AI in this world DB.
CREATE TABLE IF NOT EXISTS `_backup_20260918_when_all_aligned_objective` LIKE `quest_objectives`;

INSERT IGNORE INTO `_backup_20260918_when_all_aligned_objective`
SELECT *
FROM `quest_objectives`
WHERE `QuestID` = 35704
  AND `ObjectID` = 82651;

UPDATE `quest_objectives`
SET `ObjectID` = 77857
WHERE `QuestID` = 35704
  AND `ObjectID` = 82651;
