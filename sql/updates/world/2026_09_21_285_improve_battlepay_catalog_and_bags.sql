-- Give every BattlePay product its real Legion 7.3.5 item icon and describe
-- exactly what is delivered. Also add the largest bags natively supported by
-- build 26972. The 36-slot bags are profession-specific; Madman's Luggage is
-- the largest general-purpose bag in this client (34 slots, unique).

START TRANSACTION;

UPDATE `battlepay_display_info`
SET `FileDataID` = CASE `DisplayInfoId`
        WHEN 1 THEN 369225
        WHEN 2 THEN 526356
        WHEN 3 THEN 576138
        WHEN 4 THEN 841541
        WHEN 5 THEN 943739
        WHEN 6 THEN 986419
        WHEN 7 THEN 986420
        WHEN 8 THEN 1096090
        WHEN 9 THEN 1616898
        WHEN 10 THEN 327735
        WHEN 11 THEN 328270
        WHEN 12 THEN 656241
        WHEN 13 THEN 574806
        WHEN 14 THEN 874857
        WHEN 15 THEN 132620
        WHEN 16 THEN 1116250
        WHEN 17 THEN 1029751
        WHEN 18 THEN 1379173
    END,
    `Name2` = CASE `DisplayInfoId`
        WHEN 1 THEN 'You receive: 1x Celestial Steed'
        WHEN 2 THEN 'You receive: 1x Winged Guardian'
        WHEN 3 THEN 'You receive: 1x Heart of the Aspects'
        WHEN 4 THEN 'You receive: 1x Enchanted Fey Dragon'
        WHEN 5 THEN 'You receive: 1x Iron Skyreaver'
        WHEN 6 THEN 'You receive: 1x Warforged Nightmare'
        WHEN 7 THEN 'You receive: 1x Grinning Reaver'
        WHEN 8 THEN 'You receive: 1x Mystic Runesaber'
        WHEN 9 THEN 'You receive: 1x Luminous Starseeker'
        WHEN 10 THEN 'You receive: 1x Gryphon Hatchling'
        WHEN 11 THEN 'You receive: 1x Wind Rider Cub'
        WHEN 12 THEN 'You receive: 1x Cenarion Hatchling'
        WHEN 13 THEN 'You receive: 1x Soul of the Aspects'
        WHEN 14 THEN 'You receive: 1x Blossoming Ancient'
        WHEN 15 THEN 'You receive: 1x Alterac Brandy'
        WHEN 16 THEN 'You receive: 1x Brightpaw'
        WHEN 17 THEN 'You receive: 1x Hexweave Bag (30 slots)'
        WHEN 18 THEN 'You receive: 4x Imbued Silkweave Bag (30 slots each)'
    END,
    `Name3` = CASE `DisplayInfoId`
        WHEN 1 THEN 'Use item 54811 to learn the Celestial Steed account-wide mount.'
        WHEN 2 THEN 'Use item 69846 to learn the Winged Guardian account-wide mount.'
        WHEN 3 THEN 'Use item 78924 to learn the Heart of the Aspects account-wide mount.'
        WHEN 4 THEN 'Use item 97989 to learn the Enchanted Fey Dragon account-wide mount.'
        WHEN 5 THEN 'Use item 107951 to learn the Iron Skyreaver account-wide mount.'
        WHEN 6 THEN 'Use item 112326 to learn the Warforged Nightmare account-wide mount.'
        WHEN 7 THEN 'Use item 112327 to learn the Grinning Reaver account-wide mount.'
        WHEN 8 THEN 'Use item 122469 to learn the Mystic Runesaber account-wide mount.'
        WHEN 9 THEN 'Use item 147901 to learn the Luminous Starseeker account-wide mount.'
        WHEN 10 THEN 'Use item 49662 to add the Gryphon Hatchling battle pet to your collection.'
        WHEN 11 THEN 'Use item 49663 to add the Wind Rider Cub battle pet to your collection.'
        WHEN 12 THEN 'Use item 70099 to add the Cenarion Hatchling battle pet to your collection.'
        WHEN 13 THEN 'Use item 78916 to add the Soul of the Aspects battle pet to your collection.'
        WHEN 14 THEN 'Use item 98550 to add the Blossoming Ancient battle pet to your collection.'
        WHEN 15 THEN 'Use item 106240 to add the Alterac Brew-Pup battle pet to your collection.'
        WHEN 16 THEN 'Use item 128424 to add the Brightpaw battle pet to your collection.'
        WHEN 17 THEN 'General-purpose 30-slot bag. Holds any normal item.'
        WHEN 18 THEN 'Repeatable four-bag bundle. Each general-purpose 30-slot bag holds any normal item.'
    END
WHERE `DisplayInfoId` BETWEEN 1 AND 18;

-- Large bags are highlighted with the orange legendary shop color. This is
-- presentation only; the underlying retail item quality remains unchanged.
UPDATE `battlepay_display_info`
SET `Name1` = CASE `DisplayInfoId`
        WHEN 17 THEN '|cffff8000Hexweave Bag|r'
        WHEN 18 THEN '|cffff8000Imbued Silkweave Bag x4|r'
    END
WHERE `DisplayInfoId` IN (17, 18);

-- Correct two catalog links that pointed to unrelated Legion items.
UPDATE `battlepay_product_item` SET `ItemID` = 112327 WHERE `ProductID` = 7;
UPDATE `battlepay_product_item` SET `ItemID` = 106240 WHERE `ProductID` = 15;
UPDATE `battlepay_product_item` SET `Quantity` = 4 WHERE `ProductID` = 18 AND `ItemID` = 142075;

INSERT INTO `battlepay_display_info`
(`DisplayInfoId`, `CreatureDisplayInfoID`, `FileDataID`, `Flags`, `Name1`, `Name2`, `Name3`, `Name4`) VALUES
(19,0,647735,0,'|cffff8000Madman''s Luggage|r','You receive: 1x Madman''s Luggage (34 slots)','Largest general-purpose bag in Legion 7.3.5. Unique: only one may be carried.',''),
(20,0,466035,0,'|cffff8000Lure Master Tackle Box|r','You receive: 1x 36-slot tackle box','Fishing supplies only. This is not a general-purpose bag.',''),
(21,0,348521,0,'|cffff8000Otherworldly Bag|r','You receive: 1x 36-slot enchanting bag','Enchanting materials only. This is not a general-purpose bag.',''),
(22,0,133655,0,'|cffff8000Luxurious Silk Gem Bag|r','You receive: 1x 36-slot gem bag','Gems and jewelcrafting supplies only. This is not a general-purpose bag.',''),
(23,0,348520,0,'|cffff8000Hyjal Expedition Bag|r','You receive: 1x 36-slot herb bag','Herbs only. This is not a general-purpose bag.',''),
(24,0,465841,0,'|cffff8000Elementium Toolbox|r','You receive: 1x 36-slot engineering bag','Engineering supplies only. This is not a general-purpose bag.',''),
(25,0,348526,0,'|cffff8000Royal Scribe''s Satchel|r','You receive: 1x 36-slot inscription bag','Inscription supplies only. This is not a general-purpose bag.',''),
(26,0,348519,0,'|cffff8000Triple-Reinforced Mining Bag|r','You receive: 1x 36-slot mining bag','Mining supplies only. This is not a general-purpose bag.',''),
(27,0,133633,0,'|cffff8000Magnificent Hide Pack|r','You receive: 1x 36-slot leatherworking bag','Leatherworking supplies only. This is not a general-purpose bag.','')
ON DUPLICATE KEY UPDATE
`CreatureDisplayInfoID` = VALUES(`CreatureDisplayInfoID`), `FileDataID` = VALUES(`FileDataID`),
`Flags` = VALUES(`Flags`), `Name1` = VALUES(`Name1`), `Name2` = VALUES(`Name2`),
`Name3` = VALUES(`Name3`), `Name4` = VALUES(`Name4`);

INSERT INTO `battlepay_product`
(`ProductID`, `NormalPriceFixedPoint`, `CurrentPriceFixedPoint`, `Type`, `ChoiceType`, `Flags`, `DisplayInfoID`, `ScriptName`, `ClassMask`, `WebsiteType`) VALUES
(19,10,10,0,0,0,19,'',0,3),
(20,5,5,0,0,0,20,'',0,3),
(21,5,5,0,0,0,21,'',0,3),
(22,5,5,0,0,0,22,'',0,3),
(23,5,5,0,0,0,23,'',0,3),
(24,5,5,0,0,0,24,'',0,3),
(25,5,5,0,0,0,25,'',0,3),
(26,5,5,0,0,0,26,'',0,3),
(27,5,5,0,0,0,27,'',0,3)
ON DUPLICATE KEY UPDATE
`NormalPriceFixedPoint` = VALUES(`NormalPriceFixedPoint`), `CurrentPriceFixedPoint` = VALUES(`CurrentPriceFixedPoint`),
`Type` = VALUES(`Type`), `ChoiceType` = VALUES(`ChoiceType`), `Flags` = VALUES(`Flags`),
`DisplayInfoID` = VALUES(`DisplayInfoID`), `ScriptName` = VALUES(`ScriptName`),
`ClassMask` = VALUES(`ClassMask`), `WebsiteType` = VALUES(`WebsiteType`);

INSERT INTO `battlepay_product_item`
(`ID`, `ProductID`, `ItemID`, `Quantity`, `DisplayID`, `PetResult`) VALUES
(19,19,138300,1,0,0),
(20,20,60218,1,0,0),
(21,21,54445,1,0,0),
(22,22,70138,1,0,0),
(23,23,54446,1,0,0),
(24,24,60217,1,0,0),
(25,25,70136,1,0,0),
(26,26,70137,1,0,0),
(27,27,95536,1,0,0)
ON DUPLICATE KEY UPDATE
`ProductID` = VALUES(`ProductID`), `ItemID` = VALUES(`ItemID`), `Quantity` = VALUES(`Quantity`),
`DisplayID` = VALUES(`DisplayID`), `PetResult` = VALUES(`PetResult`);

INSERT INTO `battlepay_shop_entry`
(`EntryID`, `GroupID`, `ProductID`, `Ordering`, `Flags`, `BannerType`, `DisplayInfoID`) VALUES
(19,11,19,3,0,0,0),
(20,11,20,4,0,0,0),
(21,11,21,5,0,0,0),
(22,11,22,6,0,0,0),
(23,11,23,7,0,0,0),
(24,11,24,8,0,0,0),
(25,11,25,9,0,0,0),
(26,11,26,10,0,0,0),
(27,11,27,11,0,0,0)
ON DUPLICATE KEY UPDATE
`GroupID` = VALUES(`GroupID`), `ProductID` = VALUES(`ProductID`), `Ordering` = VALUES(`Ordering`),
`Flags` = VALUES(`Flags`), `BannerType` = VALUES(`BannerType`), `DisplayInfoID` = VALUES(`DisplayInfoID`);

COMMIT;
