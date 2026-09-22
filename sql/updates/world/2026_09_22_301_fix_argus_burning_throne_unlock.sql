-- Antorus has two mutually exclusive introduction quests. The full Argus
-- campaign uses the Crown of the Triumvirate (48203); the shorter fallback
-- follows An Offering of Light (49014). Both are offered by High Exarch Turalyon.
INSERT INTO `quest_template_addon`
    (`ID`, `PrevQuestID`, `NextQuestID`, `ExclusiveGroup`)
VALUES
    (48203, 47654, 49015, 48203)
ON DUPLICATE KEY UPDATE
    `PrevQuestID` = VALUES(`PrevQuestID`),
    `NextQuestID` = VALUES(`NextQuestID`),
    `ExclusiveGroup` = VALUES(`ExclusiveGroup`);

UPDATE `quest_template_addon`
SET
    `PrevQuestID` = 48559,
    `NextQuestID` = 49015,
    `ExclusiveGroup` = 48203
WHERE `ID` = 49014;

-- The fallback is offered by the earlier Vindicaar Turalyon spawn; the full
-- campaign version is offered by his later Antoran Wastes spawn.
DELETE FROM `creature_queststarter`
WHERE `quest` = 49014 AND `id` = 126954;

INSERT IGNORE INTO `creature_queststarter` (`id`, `quest`)
VALUES
    (126954, 48203),
    (124312, 49014);

-- Quest-accept conditions are also evaluated for the quest marker. Keep the
-- fallback hidden after the full campaign ending has been rewarded.
DELETE FROM `conditions`
WHERE `SourceTypeOrReferenceId` IN (19, 20)
  AND `SourceEntry` IN (48203, 49014);

INSERT INTO `conditions`
    (`SourceTypeOrReferenceId`, `SourceGroup`, `SourceEntry`, `SourceId`,
     `ElseGroup`, `ConditionTypeOrReference`, `ConditionTarget`,
     `ConditionValue1`, `ConditionValue2`, `ConditionValue3`,
     `NegativeCondition`, `ErrorTextId`, `ScriptName`, `Comment`)
VALUES
    (19, 0, 48203, 0, 0, 8, 0, 47654, 0, 0, 0, 0, '',
     'The Burning Throne (full) requires The Crest of Knowledge rewarded'),
    (19, 0, 49014, 0, 0, 8, 0, 48559, 0, 0, 0, 0, '',
     'The Burning Throne (fallback) requires An Offering of Light rewarded'),
    (19, 0, 49014, 0, 0, 8, 0, 47654, 0, 0, 1, 0, '',
     'The Burning Throne (fallback) requires the full Argus ending not rewarded');
