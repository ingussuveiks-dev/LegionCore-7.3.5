-- Restore the remaining Argus profession, rare, elite, and dungeon world
-- quests from the 7.3.5.26972 QuestV2CliTask WorldStateExpression data.
DELETE FROM `world_quest_update`
WHERE `QuestID` IN
    (48318, 48323, 48363, 48364, 48373, 48624, 48723, 48780, 48959);

INSERT INTO `world_quest_update`
    (`QuestID`, `Timer`, `VariableID`, `VariableID1`, `Value`, `Value1`,
     `AreaID`, `EventID`, `VerifiedBuild`)
VALUES
    (48318,  86400, 13958, 0, 1, 0, '', 0, 26124),
    (48323,  86400, 13979, 0, 1, 0, '', 0, 26124),
    (48363,  86400, 13980, 0, 1, 0, '', 0, 26124),
    (48364,  86400, 13981, 0, 1, 0, '', 0, 26124),
    (48373,  86400, 13986, 0, 1, 0, '', 0, 26124),
    (48624,  43200, 14133, 0, 1, 0, '', 0, 26124),
    (48723,  86400, 14173, 0, 1, 0, '', 0, 26124),
    (48780,  43200, 14192, 0, 1, 0, '', 0, 26124),
    (48959, 604800, 14290, 0, 1, 0, '', 0, 26124);

-- QuestInfo 118 (Alchemy world quests) was the only Legion crafting type
-- without a reward template, causing every Alchemy world quest to be skipped
-- while world_quest_update was loaded. The generic pool mirrors the other
-- crafting professions. Argus has an exact-zone override because its two work
-- orders have fixed 7.3 rewards below rather than a generated Broken Isles
-- material reward.
INSERT INTO `world_quest_template`
    (`QuestInfoID`, `ZoneID`, `PrimaryID`, `MinItemLevel`, `BonusLevel`,
     `modTreeID`, `Chance`, `Min`, `Max`, `AllMax`, `CurrencyID`,
     `CurrencyID_A`, `CurrencyID_H`, `CurrencyMin`, `CurrencyMax`, `GoldMin`,
     `GoldMax`, `ItemCAList`, `ItemResourceList`, `ArmorList`, `HasArmor`,
     `Currency`, `CurrencyCount`, `IsPvP`, `Comment`)
VALUES
    (118, 0, 0, 0, 5, 25, 50, 1, 2, 4, 0, 0, 0, 0, 0, 0, 0, '',
     '124124 2 10 59 124104 30 30 7 124103 30 30 7 124105 10 10 7 124102 30 30 7 128304 50 50 7 124101 30 30 7 ',
     '', 0, 0, 0, 0, 'Alchemy world quest'),
    (118, 8574, 0, 0, 5, 25, 50, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, '', '',
     '', 0, 0, 0, 0, 'Alchemy world quest - Krokuun')
ON DUPLICATE KEY UPDATE
    `PrimaryID` = VALUES(`PrimaryID`),
    `MinItemLevel` = VALUES(`MinItemLevel`),
    `BonusLevel` = VALUES(`BonusLevel`),
    `modTreeID` = VALUES(`modTreeID`),
    `Chance` = VALUES(`Chance`),
    `Min` = VALUES(`Min`),
    `Max` = VALUES(`Max`),
    `AllMax` = VALUES(`AllMax`),
    `CurrencyID` = VALUES(`CurrencyID`),
    `CurrencyID_A` = VALUES(`CurrencyID_A`),
    `CurrencyID_H` = VALUES(`CurrencyID_H`),
    `CurrencyMin` = VALUES(`CurrencyMin`),
    `CurrencyMax` = VALUES(`CurrencyMax`),
    `GoldMin` = VALUES(`GoldMin`),
    `GoldMax` = VALUES(`GoldMax`),
    `ItemCAList` = VALUES(`ItemCAList`),
    `ItemResourceList` = VALUES(`ItemResourceList`),
    `ArmorList` = VALUES(`ArmorList`),
    `HasArmor` = VALUES(`HasArmor`),
    `Currency` = VALUES(`Currency`),
    `CurrencyCount` = VALUES(`CurrencyCount`),
    `IsPvP` = VALUES(`IsPvP`),
    `Comment` = VALUES(`Comment`);

-- The 7.3 client quest data lists one Astral Glory and one Primal Sargerite as
-- fixed rewards for both Krokuun Alchemy work orders. The rank-two recipe is
-- conditional: the player must know rank one and must not already know rank two.
UPDATE `quest_template`
SET
    `RewardItem1` = 151565,
    `RewardAmount1` = 1,
    `RewardItem2` = 151568,
    `RewardAmount2` = 1
WHERE `ID` IN (48318, 48323);

DELETE FROM `world_quest_item`
WHERE `QuestID` IN (48318, 48323);

INSERT INTO `world_quest_item`
    (`QuestID`, `ItemID`, `ItemCount`, `NotNeedSpell`, `NeedSpell`)
VALUES
    (48318, 151658, 1, 247620, 247619),
    (48323, 151704, 1, 247690, 247688);
