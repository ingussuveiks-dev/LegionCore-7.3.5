-- Enable the Legion-era boost. The client opens its native specialization
-- picker because WebsiteType 29 and ChoiceType 2 are retained.

START TRANSACTION;

UPDATE `battlepay_display_info`
SET `Name1` = 'Level 100 Character Boost',
    `Name2` = 'Character Service',
    `Name3` = 'Boost one character to level 100. Choose a specialization and receive a matching Legion starter set.'
WHERE `DisplayInfoId` = 109;

UPDATE `battlepay_product`
SET `ScriptName` = 'battlepay_service_level100',
    `WebsiteType` = 29,
    `ChoiceType` = 2
WHERE `ProductID` = 109;

INSERT INTO `battlepay_shop_entry`
(`EntryID`, `GroupID`, `ProductID`, `Ordering`, `Flags`, `BannerType`, `DisplayInfoID`) VALUES
(109, 3, 109, 1, 0, 0, 109)
ON DUPLICATE KEY UPDATE
`GroupID` = VALUES(`GroupID`), `ProductID` = VALUES(`ProductID`), `Ordering` = VALUES(`Ordering`),
`Flags` = VALUES(`Flags`), `BannerType` = VALUES(`BannerType`), `DisplayInfoID` = VALUES(`DisplayInfoID`);

COMMIT;
