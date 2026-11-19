-- Legion 7.3.5 QuestV2CliTask and WorldStateExpression:
-- 46021: 21434 requires 13005=1 and 13084=1 during the Stormheim assault.
-- 46063: 21466 requires 13114=2, the alternative to Creepy Crawlies (13114=3).
-- 46073: 21471 requires 12978=3 in the Garrison of the Fel.
-- The loader uses the existing client area groups 4470, 5120 and 5093.
-- Use this realm's matching 6-hour assault and 24-hour Broken Shore timers.
-- Keep any rotation policy already configured for these specific quests.
INSERT INTO `world_quest_update`
    (`QuestID`, `Timer`, `VariableID`, `VariableID1`, `Value`, `Value1`, `AreaID`, `EventID`, `VerifiedBuild`)
SELECT candidate.`QuestID`, candidate.`Timer`, candidate.`VariableID`, candidate.`VariableID1`,
       candidate.`Value`, candidate.`Value1`, '', 0, 0
FROM (
    SELECT 46021 AS `QuestID`, 21600 AS `Timer`, 13005 AS `VariableID`, 13084 AS `VariableID1`, 1 AS `Value`, 1 AS `Value1`
    UNION ALL SELECT 46063, 86400, 13114, 0, 2, 0
    UNION ALL SELECT 46073, 86400, 12978, 0, 3, 0
) AS candidate
WHERE NOT EXISTS (
    SELECT 1 FROM `world_quest_update` AS existing WHERE existing.`QuestID` = candidate.`QuestID`
);
