-- Add late-Legion collectibles supported by the 7.3.5.26972 client.
-- Item IDs and preview models were checked against this build's ItemSparse,
-- ItemEffect, Mount, MountXDisplay, BattlePetSpecies, and Creature DB2 stores.
-- The newer Shu-Zen shop mount is absent from these client stores.

START TRANSACTION;

-- Put the new offers first while keeping the original catalog in order.
UPDATE `battlepay_shop_entry`
SET `Ordering` = `ProductID` + 4
WHERE `GroupID` = 2 AND `ProductID` BETWEEN 1 AND 9;

UPDATE `battlepay_shop_entry`
SET `Ordering` = `ProductID` - 6
WHERE `GroupID` = 4 AND `ProductID` BETWEEN 10 AND 16;

INSERT INTO `battlepay_display_info`
(`DisplayInfoId`, `CreatureDisplayInfoID`, `FileDataID`, `Flags`, `Name1`, `Name2`, `Name3`, `Name4`) VALUES
(147,81114,10,0,'Lightforged Felcrusher','Mount','Use item 155656 to learn the Lightforged Felcrusher mount.',''),
(148,79479,10,0,'Antoran Charhound','Mount','Use item 152816 to learn the Antoran Charhound mount.',''),
(149,79436,10,0,'Shackled Ur''zul','Mount','Use item 152789 to learn the Shackled Ur''zul mount.',''),
(150,78092,10,0,'Lucid Nightmare','Mount','Use item 151623 to learn the Lucid Nightmare mount.',''),
(151,77501,10,0,'Shadow','Battle pet','Use item 151234 to add Shadow to your battle pet collection.',''),
(152,76595,10,0,'Twilight','Battle pet','Use item 147900 to add Twilight to your battle pet collection.',''),
(153,73353,10,0,'Mischief','Battle pet','Use item 141893 to add Mischief to your battle pet collection.','')
ON DUPLICATE KEY UPDATE
`CreatureDisplayInfoID` = VALUES(`CreatureDisplayInfoID`), `FileDataID` = VALUES(`FileDataID`),
`Flags` = VALUES(`Flags`), `Name1` = VALUES(`Name1`), `Name2` = VALUES(`Name2`),
`Name3` = VALUES(`Name3`), `Name4` = VALUES(`Name4`);

-- The 7.3.5 Store UI expects scene 10 and a CreatureDisplayInfo model in
-- its separate visual row, in addition to the product display record.
DELETE FROM `battlepay_display_info_visuals` WHERE `DisplayInfoId` BETWEEN 147 AND 153;
INSERT INTO `battlepay_display_info_visuals`
(`DisplayInfoId`, `DisplayId`, `VisualId`, `ProductName`) VALUES
(147,10,81114,'Lightforged Felcrusher'),
(148,10,79479,'Antoran Charhound'),
(149,10,79436,'Shackled Ur''zul'),
(150,10,78092,'Lucid Nightmare'),
(151,10,77501,'Shadow'),
(152,10,76595,'Twilight'),
(153,10,73353,'Mischief');

INSERT INTO `battlepay_product`
(`ProductID`, `NormalPriceFixedPoint`, `CurrentPriceFixedPoint`, `Type`, `ChoiceType`, `Flags`, `DisplayInfoID`, `ScriptName`, `ClassMask`, `WebsiteType`) VALUES
(147,25,25,0,0,0,147,'',0,3),
(148,25,25,0,0,0,148,'',0,3),
(149,25,25,0,0,0,149,'',0,3),
(150,25,25,0,0,0,150,'',0,3),
(151,10,10,0,0,0,151,'',0,3),
(152,10,10,0,0,0,152,'',0,3),
(153,10,10,0,0,0,153,'',0,3)
ON DUPLICATE KEY UPDATE
`NormalPriceFixedPoint` = VALUES(`NormalPriceFixedPoint`), `CurrentPriceFixedPoint` = VALUES(`CurrentPriceFixedPoint`),
`Type` = VALUES(`Type`), `ChoiceType` = VALUES(`ChoiceType`), `Flags` = VALUES(`Flags`),
`DisplayInfoID` = VALUES(`DisplayInfoID`), `ScriptName` = VALUES(`ScriptName`),
`ClassMask` = VALUES(`ClassMask`), `WebsiteType` = VALUES(`WebsiteType`);

INSERT INTO `battlepay_product_item`
(`ID`, `ProductID`, `ItemID`, `Quantity`, `DisplayID`, `PetResult`) VALUES
(147,147,155656,1,0,0),
(148,148,152816,1,0,0),
(149,149,152789,1,0,0),
(150,150,151623,1,0,0),
(151,151,151234,1,0,0),
(152,152,147900,1,0,0),
(153,153,141893,1,0,0)
ON DUPLICATE KEY UPDATE
`ProductID` = VALUES(`ProductID`), `ItemID` = VALUES(`ItemID`),
`Quantity` = VALUES(`Quantity`), `DisplayID` = VALUES(`DisplayID`),
`PetResult` = VALUES(`PetResult`);

INSERT INTO `battlepay_shop_entry`
(`EntryID`, `GroupID`, `ProductID`, `Ordering`, `Flags`, `BannerType`, `DisplayInfoID`) VALUES
(147,2,147,1,0,0,0),
(148,2,148,2,0,0,0),
(149,2,149,3,0,0,0),
(150,2,150,4,0,0,0),
(151,4,151,1,0,0,0),
(152,4,152,2,0,0,0),
(153,4,153,3,0,0,0)
ON DUPLICATE KEY UPDATE
`GroupID` = VALUES(`GroupID`), `ProductID` = VALUES(`ProductID`),
`Ordering` = VALUES(`Ordering`), `Flags` = VALUES(`Flags`),
`BannerType` = VALUES(`BannerType`), `DisplayInfoID` = VALUES(`DisplayInfoID`);

COMMIT;
