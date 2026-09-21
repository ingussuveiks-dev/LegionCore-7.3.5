-- Document the completed Legion boost bundle. Code-side delivery mails four
-- general-purpose 30-slot bags in addition to the specialization gear.

UPDATE `battlepay_display_info`
SET `Name3` = 'Boost one character to level 100. Choose the character and specialization, receive the matching Legion starter set, plus 4x Imbued Silkweave Bag (30 slots each) by in-game mail.'
WHERE `DisplayInfoId` = 109;

-- These categories are safe before entering the world. The server validates
-- the selected character and mails item purchases to that exact character.
UPDATE `battlepay_product_group`
SET `IngameOnly` = 0
WHERE `GroupID` IN (1, 2, 3, 11);
