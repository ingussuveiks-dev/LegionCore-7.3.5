-- Legion 7.3.5 classifies paid character upgrades by both their decorator and
-- their client-known product identity: 109 is level 90 and 110 is level 100.
-- Also provide the model-scene/card records which the Store UI needs in
-- addition to CreatureDisplayInfoID.  Without them it renders an empty ring.

START TRANSACTION;

-- Move the custom level 100 service from the level 90 product slot to the
-- client-known level 100 slot.  Account entitlements are stored separately,
-- so already purchased credits remain valid after this catalog migration.
INSERT INTO `battlepay_display_info`
    (`DisplayInfoId`, `CreatureDisplayInfoID`, `FileDataID`, `Flags`, `Name1`, `Name2`, `Name3`, `Name4`)
VALUES
    (110, 0, 0, 2, 'Level 100 Character Boost', 'Character Service',
     'Boost one character to level 100. Choose the character and specialization, receive the matching Legion starter set, plus 4x Imbued Silkweave Bag (30 slots each) by in-game mail.', '')
ON DUPLICATE KEY UPDATE
    `CreatureDisplayInfoID` = VALUES(`CreatureDisplayInfoID`),
    `FileDataID` = VALUES(`FileDataID`),
    `Flags` = VALUES(`Flags`),
    `Name1` = VALUES(`Name1`),
    `Name2` = VALUES(`Name2`),
    `Name3` = VALUES(`Name3`),
    `Name4` = VALUES(`Name4`);

INSERT INTO `battlepay_product`
    (`ProductID`, `NormalPriceFixedPoint`, `CurrentPriceFixedPoint`, `Type`, `ChoiceType`, `Flags`, `DisplayInfoID`, `ScriptName`, `ClassMask`, `WebsiteType`)
VALUES
    (110, 100, 100, 0, 2, 0, 110, 'battlepay_service_level100', 0, 29)
ON DUPLICATE KEY UPDATE
    `NormalPriceFixedPoint` = VALUES(`NormalPriceFixedPoint`),
    `CurrentPriceFixedPoint` = VALUES(`CurrentPriceFixedPoint`),
    `Type` = VALUES(`Type`),
    `ChoiceType` = VALUES(`ChoiceType`),
    `Flags` = VALUES(`Flags`),
    `DisplayInfoID` = VALUES(`DisplayInfoID`),
    `ScriptName` = VALUES(`ScriptName`),
    `ClassMask` = VALUES(`ClassMask`),
    `WebsiteType` = VALUES(`WebsiteType`);

INSERT INTO `battlepay_shop_entry`
    (`EntryID`, `GroupID`, `ProductID`, `Ordering`, `Flags`, `BannerType`, `DisplayInfoID`)
VALUES
    (110, 22, 110, 1, 0, 0, 0)
ON DUPLICATE KEY UPDATE
    `GroupID` = VALUES(`GroupID`),
    `ProductID` = VALUES(`ProductID`),
    `Ordering` = VALUES(`Ordering`),
    `Flags` = VALUES(`Flags`),
    `BannerType` = VALUES(`BannerType`),
    `DisplayInfoID` = VALUES(`DisplayInfoID`);

DELETE FROM `battlepay_product_item` WHERE `ProductID` = 109;
DELETE FROM `battlepay_shop_entry` WHERE `ProductID` = 109 OR `EntryID` = 109;
DELETE FROM `battlepay_product` WHERE `ProductID` = 109;
DELETE FROM `battlepay_display_info_visuals` WHERE `DisplayInfoId` = 109;
DELETE FROM `battlepay_display_info` WHERE `DisplayInfoId` = 109;

-- The outer FileDataID is serialized as the shared ModelSceneID.  The visual
-- row becomes the Store UI card: DisplayId is CreatureDisplayInfoID and
-- VisualId is that card's ModelSceneID.
UPDATE `battlepay_display_info`
SET `FileDataID` = CASE `DisplayInfoId`
        WHEN 1  THEN 4
        WHEN 2  THEN 4
        WHEN 3  THEN 4
        WHEN 4  THEN 4
        WHEN 5  THEN 4
        WHEN 6  THEN 4
        WHEN 7  THEN 44
        WHEN 8  THEN 4
        WHEN 9  THEN 4
        WHEN 28 THEN 4
        WHEN 10 THEN 6
        WHEN 11 THEN 6
        WHEN 12 THEN 6
        WHEN 13 THEN 6
        WHEN 14 THEN 35
        WHEN 15 THEN 6
        WHEN 16 THEN 6
    END
WHERE `DisplayInfoId` IN (1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,28);

DELETE FROM `battlepay_display_info_visuals`
WHERE `DisplayInfoId` IN (1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,28);

INSERT INTO `battlepay_display_info_visuals`
    (`DisplayInfoId`, `DisplayId`, `VisualId`, `ProductName`)
VALUES
    (1,  31958, 4,  'Celestial Steed'),
    (2,  38260, 4,  'Winged Guardian'),
    (3,  40029, 4,  'Heart of the Aspects'),
    (4,  48714, 4,  'Enchanted Fey Dragon'),
    (5,  53038, 4,  'Iron Skyreaver'),
    (6,  55896, 4,  'Warforged Nightmare'),
    (7,  55907, 44, 'Grinning Reaver'),
    (8,  62148, 4,  'Mystic Runesaber'),
    (9,  76586, 4,  'Luminous Starseeker'),
    (28, 76586, 4,  'Luminous Starseeker'),
    (10, 30412, 6,  'Gryphon Hatchling'),
    (11, 30413, 6,  'Wind Rider Cub'),
    (12, 16943, 6,  'Cenarion Hatchling'),
    (13, 40019, 6,  'Soul of the Aspects'),
    (14, 48934, 35, 'Blossoming Ancient'),
    (15, 51988, 6,  'Alterac Brew-Pup'),
    (16, 64621, 6,  'Brightpaw');

COMMIT;
