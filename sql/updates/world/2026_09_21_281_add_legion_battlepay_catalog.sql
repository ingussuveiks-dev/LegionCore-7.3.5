-- Starter in-game store catalog for the Legion 7.3.5 (build 26972) client.
-- Products deliberately use items available to this client build and avoid
-- raid gear, artifact power, and later-expansion collectibles.

START TRANSACTION;

-- The stock character service is upgraded to Legion's level-100 boost by the
-- following update, after the collectible catalog has been installed.

INSERT INTO `battlepay_tokens` (`tokenType`, `name`, `loginMessage`, `listIfNone`) VALUES
(1, 'Battle Coins', 'Battle Coins can be spent through the in-game SHOP button.', 1)
ON DUPLICATE KEY UPDATE
`name` = VALUES(`name`), `loginMessage` = VALUES(`loginMessage`), `listIfNone` = VALUES(`listIfNone`);

INSERT INTO `battlepay_product_group`
(`GroupID`, `Name`, `IconFileDataID`, `DisplayType`, `Ordering`, `Flags`, `TokenType`, `IngameOnly`, `OwnsTokensOnly`) VALUES
(1,  'Mounts', 0, 0, 1, 0, 1, 1, 0),
(2,  'Pets',   0, 0, 2, 0, 1, 1, 0),
(11, 'Bags',   0, 0, 4, 0, 1, 1, 0)
ON DUPLICATE KEY UPDATE
`Name` = VALUES(`Name`), `IconFileDataID` = VALUES(`IconFileDataID`), `DisplayType` = VALUES(`DisplayType`),
`Ordering` = VALUES(`Ordering`), `Flags` = VALUES(`Flags`), `TokenType` = VALUES(`TokenType`),
`IngameOnly` = VALUES(`IngameOnly`), `OwnsTokensOnly` = VALUES(`OwnsTokensOnly`);

DELETE FROM `battlepay_product_item` WHERE `ProductID` BETWEEN 700001 AND 700018;
DELETE FROM `battlepay_shop_entry` WHERE `ProductID` BETWEEN 700001 AND 700018;
DELETE FROM `battlepay_product` WHERE `ProductID` BETWEEN 700001 AND 700018;
DELETE FROM `battlepay_display_info_locales` WHERE `Id` BETWEEN 700001 AND 700018;
DELETE FROM `battlepay_display_info_visuals` WHERE `DisplayInfoId` BETWEEN 700001 AND 700018;
DELETE FROM `battlepay_display_info` WHERE `DisplayInfoId` BETWEEN 700001 AND 700018;

INSERT INTO `battlepay_display_info`
(`DisplayInfoId`, `CreatureDisplayInfoID`, `FileDataID`, `Flags`, `Name1`, `Name2`, `Name3`, `Name4`) VALUES
(700001,0,0,0,'Celestial Steed','Mount','Account collection mount from the Legion 7.3.5 client.',''),
(700002,0,0,0,'Winged Guardian','Mount','Account collection mount from the Legion 7.3.5 client.',''),
(700003,0,0,0,'Heart of the Aspects','Mount','Account collection mount from the Legion 7.3.5 client.',''),
(700004,0,0,0,'Enchanted Fey Dragon','Mount','Account collection mount from the Legion 7.3.5 client.',''),
(700005,0,0,0,'Iron Skyreaver','Mount','Account collection mount from the Legion 7.3.5 client.',''),
(700006,0,0,0,'Warforged Nightmare','Mount','Account collection mount from the Legion 7.3.5 client.',''),
(700007,0,0,0,'Grinning Reaver','Mount','Account collection mount from the Legion 7.3.5 client.',''),
(700008,0,0,0,'Mystic Runesaber','Mount','Account collection mount from the Legion 7.3.5 client.',''),
(700009,0,0,0,'Luminous Starseeker','Mount','Account collection mount from the Legion 7.3.5 client.',''),
(700010,0,0,0,'Gryphon Hatchling','Pet','Battle pet available in the Legion 7.3.5 client.',''),
(700011,0,0,0,'Wind Rider Cub','Pet','Battle pet available in the Legion 7.3.5 client.',''),
(700012,0,0,0,'Cenarion Hatchling','Pet','Battle pet available in the Legion 7.3.5 client.',''),
(700013,0,0,0,'Soul of the Aspects','Pet','Battle pet available in the Legion 7.3.5 client.',''),
(700014,0,0,0,'Blossoming Ancient','Pet','Battle pet available in the Legion 7.3.5 client.',''),
(700015,0,0,0,'Alterac Brew-Pup','Pet','Battle pet available in the Legion 7.3.5 client.',''),
(700016,0,0,0,'Brightpaw','Pet','Battle pet available in the Legion 7.3.5 client.',''),
(700017,0,0,0,'Hexweave Bag','30-slot bag','A large Warlords of Draenor bag usable by Legion characters.',''),
(700018,0,0,0,'Imbued Silkweave Bag','30-slot bag','A large Legion tailoring bag.','');

INSERT INTO `battlepay_product`
(`ProductID`, `NormalPriceFixedPoint`, `CurrentPriceFixedPoint`, `Type`, `ChoiceType`, `Flags`, `DisplayInfoID`, `ScriptName`, `ClassMask`, `WebsiteType`) VALUES
(700001,25,25,0,1,0,700001,'',0,3),
(700002,25,25,0,1,0,700002,'',0,3),
(700003,25,25,0,1,0,700003,'',0,3),
(700004,25,25,0,1,0,700004,'',0,3),
(700005,25,25,0,1,0,700005,'',0,3),
(700006,25,25,0,1,0,700006,'',0,3),
(700007,25,25,0,1,0,700007,'',0,3),
(700008,25,25,0,1,0,700008,'',0,3),
(700009,25,25,0,1,0,700009,'',0,3),
(700010,10,10,0,1,0,700010,'',0,3),
(700011,10,10,0,1,0,700011,'',0,3),
(700012,10,10,0,1,0,700012,'',0,3),
(700013,10,10,0,1,0,700013,'',0,3),
(700014,10,10,0,1,0,700014,'',0,3),
(700015,10,10,0,1,0,700015,'',0,3),
(700016,10,10,0,1,0,700016,'',0,3),
(700017,5,5,0,1,0,700017,'',0,3),
(700018,5,5,0,1,0,700018,'',0,3);

INSERT INTO `battlepay_product_item`
(`ID`, `ProductID`, `ItemID`, `Quantity`, `DisplayID`, `PetResult`) VALUES
(700001,700001,54811,1,0,0),
(700002,700002,69846,1,0,0),
(700003,700003,78924,1,0,0),
(700004,700004,97989,1,0,0),
(700005,700005,107951,1,0,0),
(700006,700006,112326,1,0,0),
(700007,700007,116663,1,0,0),
(700008,700008,122469,1,0,0),
(700009,700009,147901,1,0,0),
(700010,700010,49662,1,0,0),
(700011,700011,49663,1,0,0),
(700012,700012,70099,1,0,0),
(700013,700013,78916,1,0,0),
(700014,700014,98550,1,0,0),
(700015,700015,106244,1,0,0),
(700016,700016,128424,1,0,0),
(700017,700017,114821,1,0,0),
(700018,700018,142075,1,0,0);

INSERT INTO `battlepay_shop_entry`
(`EntryID`, `GroupID`, `ProductID`, `Ordering`, `Flags`, `BannerType`, `DisplayInfoID`) VALUES
(700001,1,700001,1,0,0,700001),
(700002,1,700002,2,0,0,700002),
(700003,1,700003,3,0,0,700003),
(700004,1,700004,4,0,0,700004),
(700005,1,700005,5,0,0,700005),
(700006,1,700006,6,0,0,700006),
(700007,1,700007,7,0,0,700007),
(700008,1,700008,8,0,0,700008),
(700009,1,700009,9,0,0,700009),
(700010,2,700010,1,0,0,700010),
(700011,2,700011,2,0,0,700011),
(700012,2,700012,3,0,0,700012),
(700013,2,700013,4,0,0,700013),
(700014,2,700014,5,0,0,700014),
(700015,2,700015,6,0,0,700015),
(700016,2,700016,7,0,0,700016),
(700017,11,700017,1,0,0,700017),
(700018,11,700018,2,0,0,700018);

COMMIT;
