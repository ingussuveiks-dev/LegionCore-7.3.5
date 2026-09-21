-- Mirror the useful Pandaria shop navigation in the Legion 7.3.5 client.
-- VIP groups are intentionally not carried over. Every delivered item below
-- was verified against the local build-26972 Item/ItemSparse DB2 stores.

START TRANSACTION;

-- Keep the Pandaria category artwork and order, but use Legion-era contents.
INSERT INTO `battlepay_product_group`
(`GroupID`, `Name`, `IconFileDataID`, `DisplayType`, `Ordering`, `Flags`, `TokenType`, `IngameOnly`, `OwnsTokensOnly`) VALUES
(1,  'Featured',            939376, 1,  1, 0, 1, 0, 0),
(2,  'Mounts',              939379, 0,  2, 0, 1, 0, 0),
(3,  'Bags',                940857, 0,  3, 0, 1, 0, 0),
(4,  'Pets',                939380, 0,  4, 0, 1, 0, 0),
(5,  'Weapons',             940868, 0,  5, 0, 1, 0, 0),
(6,  'Armor',               940856, 0,  6, 0, 1, 0, 0),
(7,  'Reinforcements',      939378, 0,  7, 0, 1, 0, 0),
(8,  'Toys',                940867, 0,  8, 0, 1, 0, 0),
(9,  'Gold & Currency',     939377, 0,  9, 0, 1, 0, 0),
(11, 'Gaming Services',     939382, 0, 10, 0, 1, 0, 0),
(12, 'Family Heirlooms',    940862, 0, 11, 0, 1, 0, 0),
(13, 'Legion Raids',        940856, 0, 12, 0, 1, 0, 0)
ON DUPLICATE KEY UPDATE
`Name` = VALUES(`Name`), `IconFileDataID` = VALUES(`IconFileDataID`),
`DisplayType` = VALUES(`DisplayType`), `Ordering` = VALUES(`Ordering`),
`Flags` = VALUES(`Flags`), `TokenType` = VALUES(`TokenType`),
`IngameOnly` = VALUES(`IngameOnly`), `OwnsTokensOnly` = VALUES(`OwnsTokensOnly`);

-- Explicitly remove the two Pandaria VIP navigation groups if they were ever
-- imported into this database.
DELETE FROM `battlepay_shop_entry` WHERE `GroupID` IN (20, 23);
DELETE FROM `battlepay_product_group_locales` WHERE `GroupID` IN (20, 23);
DELETE FROM `battlepay_product_group` WHERE `GroupID` IN (20, 23);

-- Move the existing, already tested products to their Pandaria-style groups.
UPDATE `battlepay_shop_entry` SET `GroupID` = 2 WHERE `ProductID` BETWEEN 1 AND 9;
UPDATE `battlepay_shop_entry` SET `GroupID` = 4 WHERE `ProductID` BETWEEN 10 AND 16;
UPDATE `battlepay_shop_entry` SET `GroupID` = 3 WHERE `ProductID` BETWEEN 17 AND 27;
UPDATE `battlepay_shop_entry` SET `GroupID` = 11, `Ordering` = 1 WHERE `ProductID` = 109;

DROP TEMPORARY TABLE IF EXISTS `_battlepay_legion_catalog_288`;
CREATE TEMPORARY TABLE `_battlepay_legion_catalog_288`
(
    `ProductID` INT UNSIGNED NOT NULL,
    `GroupID` INT UNSIGNED NOT NULL,
    `Ordering` INT NOT NULL,
    `Price` BIGINT UNSIGNED NOT NULL,
    `ItemID` INT UNSIGNED NOT NULL,
    `Quantity` INT UNSIGNED NOT NULL,
    `IconFileDataID` INT UNSIGNED NOT NULL,
    `Name` VARCHAR(255) NOT NULL,
    `Subtitle` VARCHAR(255) NOT NULL,
    `Description` VARCHAR(1024) NOT NULL,
    `ClassMask` INT UNSIGNED NOT NULL DEFAULT 0,
    PRIMARY KEY (`ProductID`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

INSERT INTO `_battlepay_legion_catalog_288`
(`ProductID`, `GroupID`, `Ordering`, `Price`, `ItemID`, `Quantity`, `IconFileDataID`, `Name`, `Subtitle`, `Description`, `ClassMask`) VALUES
-- Featured products are independent entries so product-to-group resolution is unambiguous.
(28, 1, 1, 25, 147901, 1, 1616898, '|cffff8000Luminous Starseeker|r', 'Featured Legion mount', 'Learn the account-wide Luminous Starseeker mount.', 0),
(29, 1, 2, 15, 142536, 1, 1387356, '|cffff8000Memory Cube|r', 'Featured Legion toy', 'Add the Memory Cube toy to your collection.', 0),
(30, 1, 3,  5, 142075, 4, 1379173, '|cffff8000Imbued Silkweave Bag x4|r', 'Featured bag bundle', 'Receive four general-purpose 30-slot Legion bags.', 0),
(31, 1, 4, 40, 152396, 1, 1617803, '|cffff8000Arsenal: Weapons of the Lightforged|r', 'Featured Legion arsenal', 'Unlock the Weapons of the Lightforged appearances.', 0),

-- Legion weapon appearance unlocks.
(32, 5, 1, 30, 141371, 1, 1278389, '|cffa335eeArsenal: Armaments of the Silver Hand|r', 'Paladin weapon appearances', 'Unlock the Silver Hand armament appearances.', 2),
(33, 5, 2, 30, 141372, 1, 1487015, '|cffa335eeArsenal: Armaments of the Ebon Blade|r', 'Death Knight weapon appearances', 'Unlock the Ebon Blade armament appearances.', 32),
(34, 5, 3, 40, 150372, 1, 135561,  '|cffa335eeArsenal: The Warglaives of Azzinoth|r', 'Demon Hunter weapon appearances', 'Unlock the Warglaives of Azzinoth appearance arsenal.', 2048),
(35, 5, 4, 40, 152396, 1, 1617803, '|cffa335eeArsenal: Weapons of the Lightforged|r', 'Legion weapon appearances', 'Unlock the Weapons of the Lightforged appearances.', 0),

-- One Argus appearance ensemble for each armor type.
(36, 6, 1, 35, 152241, 1, 1575407, '|cffa335eeLight-Woven Triumvirate Regalia|r', 'Cloth appearance ensemble', 'Legion Argus cloth armor appearances.', 400),
(37, 6, 2, 35, 152242, 1, 1544137, '|cffa335eeBurnished Triumvirate Armor|r', 'Leather appearance ensemble', 'Legion Argus leather armor appearances.', 3592),
(38, 6, 3, 35, 152243, 1, 1574625, '|cffa335eeSterling Triumvirate Chainmail|r', 'Mail appearance ensemble', 'Legion Argus mail armor appearances.', 68),
(39, 6, 4, 35, 152244, 1, 1586623, '|cffa335eeVenerated Triumvirate Battleplate|r', 'Plate appearance ensemble', 'Legion Argus plate armor appearances.', 35),

-- Legion raid preparation consumables.
(40, 7, 1,  5, 127847, 5, 1385242, '|cff0070ddFlask of the Whispered Pact x5|r', 'Legion caster flask bundle', 'Five flasks for intellect users.', 0),
(41, 7, 2,  5, 127848, 5, 1385241, '|cff0070ddFlask of the Seventh Demon x5|r', 'Legion agility flask bundle', 'Five flasks for agility users.', 0),
(42, 7, 3,  5, 127849, 5, 1385243, '|cff0070ddFlask of the Countless Armies x5|r', 'Legion strength flask bundle', 'Five flasks for strength users.', 0),
(43, 7, 4,  5, 127850, 5, 1385240, '|cff0070ddFlask of Ten Thousand Scars x5|r', 'Legion stamina flask bundle', 'Five flasks for tanks.', 0),
(44, 7, 5,  5, 142117, 20, 967532, '|cff0070ddPotion of Prolonged Power x20|r', 'Legion combat potion bundle', 'Twenty Legion combat potions.', 0),
(45, 7, 6,  5, 140587, 20, 1118739, '|cff0070ddDefiled Augment Rune x20|r', 'Legion augment rune bundle', 'Twenty reusable-until-death stat augment consumables.', 0),
(46, 7, 7, 10, 153023, 1, 1058940, '|cffff8000Lightforged Augment Rune|r', 'Permanent Legion augment rune', 'Reusable Legion augment rune from Argus.', 0),
(47, 7, 8,  5, 141333, 5, 1495827, '|cff0070ddCodex of the Tranquil Mind x5|r', 'Group talent-change bundle', 'Five codices for changing talents while rested rules do not apply.', 0),

-- Legion-compatible toys, including late-Legion Argus rewards.
(48, 8, 1, 15, 129211, 1, 1392951, '|cff0070ddSteamy Romance Novel Kit|r', 'Legion toy', 'Add this Legion toy to your collection.', 0),
(49, 8, 2, 15, 130232, 1, 467894,  '|cff0070ddMoonfeather Statue|r', 'Legion toy', 'Add this Legion toy to your collection.', 0),
(50, 8, 3, 15, 140309, 1, 132872,  '|cff0070ddPrismatic Bauble|r', 'Legion toy', 'Add this Legion toy to your collection.', 0),
(51, 8, 4, 15, 140414, 1, 237284,  '|cff0070ddFel-Loaded Dice|r', 'Legion toy', 'Add this Legion toy to your collection.', 0),
(52, 8, 5, 15, 142341, 1, 1527090, '|cff0070ddLove Boat|r', 'Legion toy', 'Add this Legion toy to your collection.', 0),
(53, 8, 6, 15, 142536, 1, 1387356, '|cff0070ddMemory Cube|r', 'Legion toy', 'Add this Legion toy to your collection.', 0),
(54, 8, 7, 15, 151016, 1, 133731,  '|cff0070ddFractured Necrolyte Skull|r', 'Argus toy', 'Add this Argus toy to your collection.', 0),
(55, 8, 8, 15, 151652, 1, 237560,  '|cff0070ddWormhole Generator: Argus|r', 'Argus engineering toy', 'Add the Argus wormhole generator to your collection.', 0),

-- Currency-like Legion items. These are normal client items whose use grants
-- the named Legion resource, avoiding unsupported synthetic Pandaria tokens.
(56, 9, 1, 10, 142156, 1, 465841,  '|cffff8000Order Resources Cache|r', 'Legion class-hall resources', 'Open to receive Order Resources.', 0),
(57, 9, 2, 10, 124124, 10, 1417744, '|cffff8000Blood of Sargeras x10|r', 'Legion crafting currency', 'Ten Bloods of Sargeras.', 0),
(58, 9, 3, 15, 139460, 1, 133858,  '|cffff8000Seal of Broken Fate|r', 'Legion bonus-roll token', 'One Legion raid bonus-roll token.', 0),
(59, 9, 4, 10, 153202, 1, 132784,  '|cffff8000Argunite Cluster|r', 'Argus currency cache', 'Open to receive Veiled Argunite.', 0),

-- Legion 100-to-110 heirloom upgrade tokens.
(60, 12, 1, 15, 151614, 1, 960150, '|cffe5cc80Weathered Heirloom Armor Casing|r', 'Upgrade heirloom armor to level 110', 'Upgrades an heirloom armor, trinket, shield, or off-hand to scale to level 110.', 0),
(61, 12, 2, 15, 151615, 1, 353645, '|cffe5cc80Weathered Heirloom Scabbard|r', 'Upgrade an heirloom weapon to level 110', 'Upgrades an heirloom weapon to scale to level 110.', 0),

-- Complete Antorus Tier 21 token selection. Class masks prevent delivery to
-- an incompatible class even when buying from character selection.
(62, 13,  1, 25, 152518, 1, 132632, '|cffa335eeChest of the Antoran Vanquisher|r', 'Antorus Tier 21 token', 'For Death Knights, Druids, Mages, and Rogues.', 1192),
(63, 13,  2, 25, 152519, 1, 132632, '|cffa335eeChest of the Antoran Conqueror|r', 'Antorus Tier 21 token', 'For Demon Hunters, Paladins, Priests, and Warlocks.', 2322),
(64, 13,  3, 25, 152520, 1, 132632, '|cffa335eeChest of the Antoran Protector|r', 'Antorus Tier 21 token', 'For Hunters, Monks, Shamans, and Warriors.', 581),
(65, 13,  4, 25, 152521, 1, 132963, '|cffa335eeGauntlets of the Antoran Vanquisher|r', 'Antorus Tier 21 token', 'For Death Knights, Druids, Mages, and Rogues.', 1192),
(66, 13,  5, 25, 152522, 1, 132963, '|cffa335eeGauntlets of the Antoran Conqueror|r', 'Antorus Tier 21 token', 'For Demon Hunters, Paladins, Priests, and Warlocks.', 2322),
(67, 13,  6, 25, 152523, 1, 132963, '|cffa335eeGauntlets of the Antoran Protector|r', 'Antorus Tier 21 token', 'For Hunters, Monks, Shamans, and Warriors.', 581),
(68, 13,  7, 25, 152524, 1, 133126, '|cffa335eeHelm of the Antoran Vanquisher|r', 'Antorus Tier 21 token', 'For Death Knights, Druids, Mages, and Rogues.', 1192),
(69, 13,  8, 25, 152525, 1, 133126, '|cffa335eeHelm of the Antoran Conqueror|r', 'Antorus Tier 21 token', 'For Demon Hunters, Paladins, Priests, and Warlocks.', 2322),
(70, 13,  9, 25, 152526, 1, 133126, '|cffa335eeHelm of the Antoran Protector|r', 'Antorus Tier 21 token', 'For Hunters, Monks, Shamans, and Warriors.', 581),
(71, 13, 10, 25, 152527, 1, 133834, '|cffa335eeLeggings of the Antoran Vanquisher|r', 'Antorus Tier 21 token', 'For Death Knights, Druids, Mages, and Rogues.', 1192),
(72, 13, 11, 25, 152528, 1, 133834, '|cffa335eeLeggings of the Antoran Conqueror|r', 'Antorus Tier 21 token', 'For Demon Hunters, Paladins, Priests, and Warlocks.', 2322),
(73, 13, 12, 25, 152529, 1, 133834, '|cffa335eeLeggings of the Antoran Protector|r', 'Antorus Tier 21 token', 'For Hunters, Monks, Shamans, and Warriors.', 581),
(74, 13, 13, 25, 152530, 1, 135053, '|cffa335eeShoulders of the Antoran Vanquisher|r', 'Antorus Tier 21 token', 'For Death Knights, Druids, Mages, and Rogues.', 1192),
(75, 13, 14, 25, 152531, 1, 135053, '|cffa335eeShoulders of the Antoran Conqueror|r', 'Antorus Tier 21 token', 'For Demon Hunters, Paladins, Priests, and Warlocks.', 2322),
(76, 13, 15, 25, 152532, 1, 135053, '|cffa335eeShoulders of the Antoran Protector|r', 'Antorus Tier 21 token', 'For Hunters, Monks, Shamans, and Warriors.', 581);

DELETE FROM `battlepay_product_item` WHERE `ProductID` BETWEEN 28 AND 76;
DELETE FROM `battlepay_shop_entry` WHERE `EntryID` BETWEEN 28 AND 76 OR `ProductID` BETWEEN 28 AND 76;
DELETE FROM `battlepay_product` WHERE `ProductID` BETWEEN 28 AND 76;
DELETE FROM `battlepay_display_info_locales` WHERE `Id` BETWEEN 28 AND 76;
DELETE FROM `battlepay_display_info_visuals` WHERE `DisplayInfoId` BETWEEN 28 AND 76;
DELETE FROM `battlepay_display_info` WHERE `DisplayInfoId` BETWEEN 28 AND 76;

INSERT INTO `battlepay_display_info`
(`DisplayInfoId`, `CreatureDisplayInfoID`, `FileDataID`, `Flags`, `Name1`, `Name2`, `Name3`, `Name4`)
SELECT `ProductID`, 0, `IconFileDataID`, 0, `Name`, `Subtitle`, `Description`, ''
FROM `_battlepay_legion_catalog_288`;

INSERT INTO `battlepay_product`
(`ProductID`, `NormalPriceFixedPoint`, `CurrentPriceFixedPoint`, `Type`, `ChoiceType`, `Flags`, `DisplayInfoID`, `ScriptName`, `ClassMask`, `WebsiteType`)
SELECT `ProductID`, `Price`, `Price`, 0, 0, 0, `ProductID`, '', `ClassMask`, 3
FROM `_battlepay_legion_catalog_288`;

INSERT INTO `battlepay_product_item`
(`ID`, `ProductID`, `ItemID`, `Quantity`, `DisplayID`, `PetResult`)
SELECT `ProductID`, `ProductID`, `ItemID`, `Quantity`, 0, 0
FROM `_battlepay_legion_catalog_288`;

INSERT INTO `battlepay_shop_entry`
(`EntryID`, `GroupID`, `ProductID`, `Ordering`, `Flags`, `BannerType`, `DisplayInfoID`)
SELECT `ProductID`, `GroupID`, `ProductID`, `Ordering`, 0, 0, 0
FROM `_battlepay_legion_catalog_288`;

DROP TEMPORARY TABLE `_battlepay_legion_catalog_288`;

COMMIT;
