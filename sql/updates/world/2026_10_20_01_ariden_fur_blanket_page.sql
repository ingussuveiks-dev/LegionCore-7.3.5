-- The optional Fur Blanket inspection is a text object, not quest credit.
-- Restore its missing page without replacing an existing/localized page.
INSERT INTO page_text (ID, Text, NextPageID, PlayerConditionID, Flags, VerifiedBuild)
SELECT 5234, 'The bedding in the tent has been undisturbed for quite some time.', 0, 0, 0, 0
WHERE NOT EXISTS (SELECT 1 FROM page_text WHERE ID=5234);
