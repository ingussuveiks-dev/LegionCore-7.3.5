-- Keep client-facing BattlePay identifiers in the small range used by the
-- retail 7.3.5 catalog. Large synthetic identifiers can be interpreted as
-- array sizes by the client when it builds the shop model.
--
-- Shop-entry DisplayInfoID is intentionally zero. Product display data is
-- already sent in ProductInfo and Product; repeating it in ShopEntry uses an
-- optional wire structure that the 26972 client does not parse reliably.

START TRANSACTION;

DELETE FROM `battlepay_product_item` WHERE `ProductID` BETWEEN 1 AND 18;
DELETE FROM `battlepay_shop_entry` WHERE `EntryID` BETWEEN 1 AND 18 OR `ProductID` BETWEEN 1 AND 18;
DELETE FROM `battlepay_product` WHERE `ProductID` BETWEEN 1 AND 18;
DELETE FROM `battlepay_display_info_locales` WHERE `Id` BETWEEN 1 AND 18;
DELETE FROM `battlepay_display_info_visuals` WHERE `DisplayInfoId` BETWEEN 1 AND 18;
DELETE FROM `battlepay_display_info` WHERE `DisplayInfoId` BETWEEN 1 AND 18;

UPDATE `battlepay_display_info`
SET `DisplayInfoId` = `DisplayInfoId` - 700000
WHERE `DisplayInfoId` BETWEEN 700001 AND 700018;

UPDATE `battlepay_display_info_locales`
SET `Id` = `Id` - 700000
WHERE `Id` BETWEEN 700001 AND 700018;

UPDATE `battlepay_display_info_visuals`
SET `DisplayInfoId` = `DisplayInfoId` - 700000
WHERE `DisplayInfoId` BETWEEN 700001 AND 700018;

UPDATE `battlepay_product`
SET `DisplayInfoID` = `DisplayInfoID` - 700000
WHERE `DisplayInfoID` BETWEEN 700001 AND 700018;

UPDATE `battlepay_product`
SET `ProductID` = `ProductID` - 700000,
    `ChoiceType` = 0
WHERE `ProductID` BETWEEN 700001 AND 700018;

UPDATE `battlepay_product_item`
SET `ID` = `ID` - 700000,
    `ProductID` = `ProductID` - 700000
WHERE `ProductID` BETWEEN 700001 AND 700018;

UPDATE `battlepay_shop_entry`
SET `EntryID` = `EntryID` - 700000,
    `ProductID` = `ProductID` - 700000,
    `DisplayInfoID` = 0
WHERE `ProductID` BETWEEN 700001 AND 700018;

UPDATE `battlepay_shop_entry`
SET `DisplayInfoID` = 0
WHERE `ProductID` = 109;

UPDATE `battlepay_product_group`
SET `IconFileDataID` = CASE `GroupID`
    WHEN 1 THEN 841541
    WHEN 2 THEN 132599
    WHEN 11 THEN 133643
    ELSE `IconFileDataID`
END
WHERE `GroupID` IN (1, 2, 11);

COMMIT;
