-- Headed South (achievement 5292) uses Criteria.db2 ID 15993 in build 26972.
-- ID 15593 belongs to an unrelated class criterion in this client build.
DELETE FROM `achievement_criteria_data`
WHERE `criteria_id` = 15593
  AND `type` = 11
  AND `ScriptName` = 'achievement_headed_south';

DELETE FROM `achievement_criteria_data`
WHERE `criteria_id` = 15993
  AND `type` = 11;

INSERT INTO `achievement_criteria_data` (`criteria_id`, `type`, `value1`, `value2`, `ScriptName`)
VALUES (15993, 11, 0, 0, 'achievement_headed_south');
