-- The 7.3.5 client hardcodes 22 as WOW_SERVICES_CATEGORY_ID. A character
-- boost can still be purchased from another shop group, but its AVAILABLE
-- distribution is then ignored by the character-select boost-token UI.

START TRANSACTION;

INSERT INTO `battlepay_product_group`
(`GroupID`, `Name`, `IconFileDataID`, `DisplayType`, `Ordering`, `Flags`, `TokenType`, `IngameOnly`, `OwnsTokensOnly`)
VALUES (22, 'Gaming Services', 939382, 0, 10, 0, 1, 0, 0)
ON DUPLICATE KEY UPDATE
`Name` = VALUES(`Name`), `IconFileDataID` = VALUES(`IconFileDataID`),
`DisplayType` = VALUES(`DisplayType`), `Ordering` = VALUES(`Ordering`),
`Flags` = VALUES(`Flags`), `TokenType` = VALUES(`TokenType`),
`IngameOnly` = VALUES(`IngameOnly`), `OwnsTokensOnly` = VALUES(`OwnsTokensOnly`);

-- Keep the level-100 boost in the client-recognized services group even if
-- an older catalog update assigned it to another navigation category.
UPDATE `battlepay_shop_entry`
SET `GroupID` = 22
WHERE `ProductID` = 109;

DELETE FROM `battlepay_product_group_locales` WHERE `GroupID` = 11;
DELETE FROM `battlepay_product_group` WHERE `GroupID` = 11;

COMMIT;
