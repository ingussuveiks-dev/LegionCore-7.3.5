-- Native QuestV2CliTask44923 -> expression20988 requires world state12678=3.
-- Restore the missing Vrekt candidate, inheriting this server's existing
-- Helarjar duration policy from Grey Shoals. The duration is not a recovered
-- retail Vrekt timer. Do not replace an operator-supplied Vrekt configuration
-- or force/reset any currently active world quest or personal progress.
INSERT INTO `world_quest_update`
(`QuestID`,`Timer`,`VariableID`,`VariableID1`,`Value`,`Value1`,`AreaID`,`EventID`,`VerifiedBuild`)
SELECT 44923,`Timer`,12678,0,3,0,'8362',0,0
FROM `world_quest_update`
WHERE `QuestID`=44737 AND `VariableID`=12678 AND `Value`=1
  AND NOT EXISTS (SELECT 1 FROM `world_quest_update` WHERE `QuestID`=44923)
ORDER BY `Timer` DESC
LIMIT 1;
