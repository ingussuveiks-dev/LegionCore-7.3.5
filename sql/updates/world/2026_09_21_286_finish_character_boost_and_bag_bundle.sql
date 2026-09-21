-- Finish the character-select level 100 boost presentation and make the
-- repeatable general-purpose bag offer useful alongside unique item 138300.

START TRANSACTION;

UPDATE `battlepay_display_info`
SET `Name3` = 'Boost one character to level 100. After purchase, log out, select the boost token, then choose a character and specialization. The matching Legion starter set is delivered by in-game mail.'
WHERE `DisplayInfoId` = 109;

UPDATE `battlepay_display_info`
SET `Name1` = '|cffff8000Imbued Silkweave Bag x4|r',
    `Name2` = 'You receive: 4x Imbued Silkweave Bag (30 slots each)',
    `Name3` = 'Repeatable four-bag bundle. Each general-purpose 30-slot bag holds any normal item.'
WHERE `DisplayInfoId` = 18;

UPDATE `battlepay_product_item`
SET `Quantity` = 4
WHERE `ProductID` = 18 AND `ItemID` = 142075;

UPDATE `battlepay_display_info`
SET `Name2` = 'You receive: 1x Madman''s Luggage (34 slots)',
    `Name3` = 'Largest general-purpose bag in Legion 7.3.5. Unique: only one may be carried.'
WHERE `DisplayInfoId` = 19;

COMMIT;
