-- The three allied-race "Stranger in a Strange Land" variants lost their
-- emissary discovery targets during import. Restore them from each quest's
-- existing ender, following the intact Lightforged variant (quest 50313).
CREATE TABLE IF NOT EXISTS `_backup_20260918_allied_race_city_objectives`
LIKE `quest_objectives`;

REPLACE INTO `_backup_20260918_allied_race_city_objectives`
SELECT *
FROM `quest_objectives`
WHERE (`ID` = 312111 AND `QuestID` = 50303 AND `ObjectID` = 0)
   OR (`ID` = 312122 AND `QuestID` = 50305 AND `ObjectID` = 0)
   OR (`ID` = 312180 AND `QuestID` = 50319 AND `ObjectID` = 0);

UPDATE `quest_objectives`
SET `ObjectID` = 131328,
    `Flags` = 1,
    `Description` = 'Melitier Vahlouran <Nightborne Emissary>'
WHERE `ID` = 312111
  AND `QuestID` = 50303
  AND `Type` = 0
  AND `ObjectID` = 0;

UPDATE `quest_objectives`
SET `ObjectID` = 131347,
    `Flags` = 1,
    `Description` = 'Keira Onyxraven <Void Elf Emissary>'
WHERE `ID` = 312122
  AND `QuestID` = 50305
  AND `Type` = 0
  AND `ObjectID` = 0;

UPDATE `quest_objectives`
SET `ObjectID` = 131201,
    `Flags` = 1,
    `Description` = 'Halian Shlavahawk <Highmountain Emissary>'
WHERE `ID` = 312180
  AND `QuestID` = 50319
  AND `Type` = 0
  AND `ObjectID` = 0;
