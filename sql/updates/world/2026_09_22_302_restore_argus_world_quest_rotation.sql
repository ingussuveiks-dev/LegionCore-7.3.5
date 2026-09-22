-- These Argus world quests have complete quest/objective data but were absent
-- from the server rotation. VariableID and Value come from the corresponding
-- 7.3.5.26972 QuestV2CliTask WorldStateExpression records.
DELETE FROM `world_quest_update`
WHERE `QuestID` IN
    (47135, 47456, 47496, 48097, 48103, 48175, 48285, 48338, 48358,
     48360, 48374, 48386, 48592);

INSERT INTO `world_quest_update`
    (`QuestID`, `Timer`, `VariableID`, `VariableID1`, `Value`, `Value1`,
     `AreaID`, `EventID`, `VerifiedBuild`)
VALUES
    (47135, 86400, 13684, 0, 1, 0, '', 0, 26124),
    (47456, 86400, 13557, 0, 1, 0, '', 0, 26124),
    (47496, 86400, 13693, 0, 1, 0, '', 0, 26124),
    (48097, 86400, 13989, 0, 1, 0, '', 0, 26124),
    (48103, 86400, 13989, 0, 3, 0, '', 0, 26124),
    (48175, 86400, 14000, 0, 2, 0, '', 0, 26124),
    (48285, 86400, 13621, 0, 4, 0, '', 0, 26124),
    (48338, 86400, 13961, 0, 1, 0, '', 0, 26124),
    (48358, 86400, 13975, 0, 1, 0, '', 0, 26124),
    (48360, 86400, 13977, 0, 1, 0, '', 0, 26124),
    (48374, 86400, 13987, 0, 1, 0, '', 0, 26124),
    (48386, 86400, 14285, 0, 1, 0, '', 0, 26124),
    (48592, 86400, 14130, 0, 1, 0, '', 0, 26124);
