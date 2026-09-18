-- Borrowed Time asks the player to give the elixir to Shinfel Blightsworn.
-- The quest gossip uses her 7.3.5 entry 104820; 104824 is Ernest Carlisle.
CREATE TABLE IF NOT EXISTS `_backup_20260918_borrowed_time_objective` LIKE `quest_objectives`;

INSERT IGNORE INTO `_backup_20260918_borrowed_time_objective`
SELECT *
FROM `quest_objectives`
WHERE `QuestID` = 41784
  AND `ObjectID` = 104824;

UPDATE `quest_objectives`
SET `ObjectID` = 104820
WHERE `QuestID` = 41784
  AND `ObjectID` = 104824;
