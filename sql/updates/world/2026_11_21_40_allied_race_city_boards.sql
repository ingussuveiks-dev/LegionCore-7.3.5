-- Match the archived 26124 objective text to the two boards beside the emissaries.
UPDATE `quest_objectives` AS `o`
JOIN `quest_objectives_locale` AS `l`
  ON `l`.`ID` = `o`.`ID` AND `l`.`QuestId` = `o`.`QuestID` AND `l`.`locale` = 'enUS'
SET `o`.`Type` = 2, `o`.`Amount` = 1, `o`.`Description` = `l`.`Description`,
    `o`.`ObjectID` = CASE WHEN `o`.`QuestID` IN (50303, 50319) THEN 206109 ELSE 206294 END
WHERE `o`.`Type` = 0 AND `o`.`Amount` = 0 AND `o`.`StorageIndex` = 0
  AND (`o`.`QuestID`, `o`.`ID`, `o`.`ObjectID`) IN
      ((50303, 312111, 131328), (50305, 312122, 131347), (50313, 312152, 131334), (50319, 312180, 131201));

UPDATE `gameobject_template` SET `ScriptName` = 'go_allied_race_city_board'
WHERE `entry` IN (206109, 206294) AND `type` = 2 AND `AIName` = '' AND `ScriptName` = '';

-- Retain the original English fallback from the local 26124 locale rows.
UPDATE `quest_template` AS `q`
JOIN `quest_template_locale` AS `l` ON `l`.`ID` = `q`.`ID` AND `l`.`locale` = 'enUS' AND `l`.`VerifiedBuild` = 26124
SET `q`.`LogTitle` = COALESCE(NULLIF(`q`.`LogTitle`, ''), `l`.`LogTitle`),
    `q`.`LogDescription` = COALESCE(NULLIF(`q`.`LogDescription`, ''), `l`.`LogDescription`),
    `q`.`QuestDescription` = COALESCE(NULLIF(`q`.`QuestDescription`, ''), `l`.`QuestDescription`),
    `q`.`QuestCompletionLog` = COALESCE(NULLIF(`q`.`QuestCompletionLog`, ''), `l`.`QuestCompletionLog`)
WHERE `q`.`ID` IN (49772, 49773, 49788, 49933, 50303, 50305, 50313, 50319);

-- Repoint the objective POI to the actual board; keep each starter and receiver POI.
UPDATE `quest_poi_points` AS `p`
JOIN `gameobject` AS `g`
  ON `g`.`guid` = CASE WHEN `p`.`QuestID` IN (50303, 50319) THEN 107487 ELSE 108162 END
SET `p`.`X` = ROUND(`g`.`position_x`), `p`.`Y` = ROUND(`g`.`position_y`), `p`.`VerifiedBuild` = 0
WHERE `p`.`QuestID` IN (50303, 50305, 50313, 50319) AND `p`.`Idx1` = 1 AND `p`.`Idx2` = 0;

-- Two variants are missing all three points. Use their existing spawn positions.
INSERT INTO `quest_poi_points` (`QuestID`, `Idx1`, `Idx2`, `X`, `Y`, `VerifiedBuild`)
SELECT `source`.`QuestID`, `source`.`Idx1`, 0, `source`.`X`, `source`.`Y`, 0
FROM (
  SELECT CASE `g`.`guid` WHEN 107487 THEN 50303 ELSE 50313 END AS `QuestID`, 1 AS `Idx1`,
         ROUND(`g`.`position_x`) AS `X`, ROUND(`g`.`position_y`) AS `Y`
  FROM `gameobject` AS `g` WHERE `g`.`guid` IN (107487, 108162)
  UNION ALL
  SELECT CASE WHEN `c`.`guid` IN (14556699, 14556700) THEN 50303 ELSE 50313 END,
         CASE WHEN `c`.`guid` IN (14556699, 14556705) THEN 0 ELSE 2 END,
         ROUND(`c`.`position_x`), ROUND(`c`.`position_y`)
  FROM `creature` AS `c` WHERE `c`.`guid` IN (14556699, 14556700, 14556702, 14556705)
) AS `source`
LEFT JOIN `quest_poi_points` AS `existing`
  ON `existing`.`QuestID` = `source`.`QuestID` AND `existing`.`Idx1` = `source`.`Idx1` AND `existing`.`Idx2` = 0
WHERE `existing`.`QuestID` IS NULL;
