-- Revered Among Thrallmar was removed from the game long before Legion. Its
-- old Hellfire Citadel AreaTable category 3535 is absent from the 7.3.5 client,
-- and this database has no starter, ender, or objective for the quest.
CREATE TABLE IF NOT EXISTS `_backup_20260918_obsolete_quest_zone`
LIKE `quest_template`;

REPLACE INTO `_backup_20260918_obsolete_quest_zone`
SELECT *
FROM `quest_template`
WHERE `ID` = 10559
  AND `QuestSortID` = 3535;

UPDATE `quest_template`
SET `QuestSortID` = 0
WHERE `ID` = 10559
  AND `QuestSortID` = 3535;
