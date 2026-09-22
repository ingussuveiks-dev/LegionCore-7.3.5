-- All twelve Legion class-mount breadcrumbs unlock after the character
-- completes the Legionfall campaign. They were incorrectly gated by Broken
-- Isles Pathfinder, Part One (11190) instead of Breaching the Tomb (11546).
UPDATE `conditions`
SET
    `ConditionValue1` = 11546,
    `Comment` = 'Legion class mount breadcrumb requires Breaching the Tomb'
WHERE `SourceTypeOrReferenceId` = 19
  AND `SourceGroup` = 0
  AND `SourceEntry` IN
      (45788, 45844, 46069, 46103, 46208, 46237,
       46317, 46333, 46336, 46353, 46719, 46791)
  AND `ConditionTypeOrReference` = 17
  AND `ConditionValue1` = 11190;

-- The final warlock quest was the only class-mount finale without addon
-- progression data. Tie it to The Dreadlord's Calling and restrict the
-- otherwise directly offered quest to warlocks.
INSERT INTO `quest_template_addon`
    (`ID`, `AllowableClasses`, `PrevQuestID`)
VALUES
    (46243, 256, 46242)
ON DUPLICATE KEY UPDATE
    `AllowableClasses` = VALUES(`AllowableClasses`),
    `PrevQuestID` = VALUES(`PrevQuestID`);
