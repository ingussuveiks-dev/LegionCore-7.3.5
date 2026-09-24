-- These quests already have rotation rows, client tasks, objectives and
-- spawned targets, but no reward template for their QuestSortID subzone.
-- Reuse the corresponding parent-zone reward pool. Keep one quest from each
-- subzone active at a time so the existing rotation remains selective.
INSERT INTO `world_quest_template`
    (`QuestInfoID`, `ZoneID`, `PrimaryID`, `MinItemLevel`, `BonusLevel`,
     `modTreeID`, `Chance`, `Min`, `Max`, `AllMax`, `CurrencyID`,
     `CurrencyID_A`, `CurrencyID_H`, `CurrencyMin`, `CurrencyMax`,
     `GoldMin`, `GoldMax`, `ItemCAList`, `ItemResourceList`, `ArmorList`,
     `HasArmor`, `Currency`, `CurrencyCount`, `IsPvP`, `Comment`)
SELECT
    missing.`QuestInfoID`, missing.`ZoneID`, source.`PrimaryID`,
    source.`MinItemLevel`, source.`BonusLevel`, source.`modTreeID`,
    100, 1, 1, 1, source.`CurrencyID`, source.`CurrencyID_A`,
    source.`CurrencyID_H`, source.`CurrencyMin`, source.`CurrencyMax`,
    source.`GoldMin`, source.`GoldMax`, source.`ItemCAList`,
    source.`ItemResourceList`, source.`ArmorList`, source.`HasArmor`,
    source.`Currency`, source.`CurrencyCount`, source.`IsPvP`,
    missing.`Comment`
FROM
    (SELECT 109 AS `QuestInfoID`, 7796 AS `ZoneID`, 7543 AS `ParentZoneID`,
            'World quest - Broken Shore subzone' AS `Comment`
     UNION ALL
     SELECT 136, 8000, 7334, 'Rare elite world quests - Azsuna subzone'
     UNION ALL
     SELECT 136, 8553, 7543, 'Rare elite world quest - Broken Shore subzone') AS missing
JOIN `world_quest_template` AS source
  ON source.`QuestInfoID` = missing.`QuestInfoID`
 AND source.`ZoneID` = missing.`ParentZoneID`
ON DUPLICATE KEY UPDATE
    `Chance` = VALUES(`Chance`),
    `Min` = VALUES(`Min`),
    `Max` = VALUES(`Max`),
    `AllMax` = VALUES(`AllMax`),
    `ItemCAList` = VALUES(`ItemCAList`),
    `Comment` = VALUES(`Comment`);

-- AddWorldQuestTask searches spawned creatures by QuestSortID. The Azsuna
-- subzone quests have QuestSortID 8000, but their creature.zoneId is 7334.
-- AreaID makes the task available where each target actually spawns.
UPDATE `world_quest_update`
SET `AreaID` = CASE `QuestID`
    WHEN 43027 THEN '7362'
    WHEN 43059 THEN '7334'
    WHEN 43063 THEN '7607'
    WHEN 43079 THEN '8193'
    WHEN 43121 THEN '8166'
    WHEN 43175 THEN '7693'
    WHEN 44187 THEN '8147'
    WHEN 44189 THEN '8147'
    WHEN 44192 THEN '8147'
END
WHERE `QuestID` IN
    (43027, 43059, 43063, 43079, 43121, 43175, 44187, 44189, 44192);
