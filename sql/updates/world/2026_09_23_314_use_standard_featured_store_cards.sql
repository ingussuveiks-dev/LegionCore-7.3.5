-- The stock 7.3.5 SplashSecondary layout places texture icons beside their
-- circular borders. Use the supported default card layout for Featured so
-- both x86/x64 clients align icons correctly without overrideArchive.
-- Preserve every product, price, category label and ordering.
UPDATE `battlepay_product_group`
SET `DisplayType` = 0
WHERE `GroupID` = 1;
