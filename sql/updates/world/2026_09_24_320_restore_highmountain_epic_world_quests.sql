-- Both Highmountain epic world quests have valid 7.3.5 client tasks,
-- kill objectives and spawned targets. Without a reward template they are
-- skipped while world_quest_update is loaded.
INSERT INTO `world_quest_template`
    (`QuestInfoID`, `ZoneID`, `PrimaryID`, `MinItemLevel`, `BonusLevel`,
     `modTreeID`, `Chance`, `Min`, `Max`, `AllMax`, `CurrencyID`,
     `CurrencyID_A`, `CurrencyID_H`, `CurrencyMin`, `CurrencyMax`,
     `GoldMin`, `GoldMax`, `ItemCAList`, `ItemResourceList`, `ArmorList`,
     `HasArmor`, `Currency`, `CurrencyCount`, `IsPvP`, `Comment`)
SELECT
    110, 7503, `PrimaryID`, `MinItemLevel`, `BonusLevel`, `modTreeID`,
    100, 1, 1, 1, `CurrencyID`, `CurrencyID_A`, `CurrencyID_H`,
    `CurrencyMin`, `CurrencyMax`, `GoldMin`, `GoldMax`, `ItemCAList`,
    `ItemResourceList`, `ArmorList`, `HasArmor`, `Currency`,
    `CurrencyCount`, `IsPvP`, 'Epic world quest - Highmountain'
FROM `world_quest_template`
WHERE `QuestInfoID` = 111 AND `ZoneID` = 7503
ON DUPLICATE KEY UPDATE
    `Chance` = VALUES(`Chance`),
    `Min` = VALUES(`Min`),
    `Max` = VALUES(`Max`),
    `AllMax` = VALUES(`AllMax`),
    `ItemCAList` = VALUES(`ItemCAList`),
    `Comment` = VALUES(`Comment`);
