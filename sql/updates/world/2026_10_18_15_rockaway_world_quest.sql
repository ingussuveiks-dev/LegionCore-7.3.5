-- Native QuestV2CliTask44746 -> expression20875 requires world state12678=2.
-- AreaGroupID is zero and objective marker115168 has no static spawns:
-- explicitly register native Rockaway Coast area7833 for task arrival.
-- Inherit this server's Grey Shoals Helarjar duration policy, not a recovered
-- retail Rockaway timer. Preserve operator configuration and active progress.
INSERT INTO `world_quest_update`
(`QuestID`,`Timer`,`VariableID`,`VariableID1`,`Value`,`Value1`,`AreaID`,`EventID`,`VerifiedBuild`)
SELECT 44746,`Timer`,12678,0,2,0,'7833',0,0
FROM `world_quest_update`
WHERE `QuestID`=44737 AND `VariableID`=12678 AND `Value`=1
  AND NOT EXISTS (SELECT 1 FROM `world_quest_update` WHERE `QuestID`=44746)
ORDER BY `Timer` DESC
LIMIT 1;
