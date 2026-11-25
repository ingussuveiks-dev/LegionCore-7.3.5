-- Quest chain links are alternatives. Liang needs both completed tasks before granting his pearl.
INSERT INTO `conditions` (`SourceTypeOrReferenceId`, `SourceGroup`, `SourceEntry`, `SourceId`, `ElseGroup`,
    `ConditionTypeOrReference`, `ConditionTarget`, `ConditionValue1`, `ConditionValue2`, `ConditionValue3`,
    `NegativeCondition`, `ErrorTextId`, `ScriptName`, `Comment`)
SELECT 19, 0, 29678, 0, 0, 8, 0, prerequisite.`ID`, 0, 0, 0, 0, '',
    CONCAT('Shu, the Spirit of Water requires ', prerequisite.`LogTitle`, ' rewarded')
FROM `quest_template` AS prerequisite
JOIN `quest_template_addon` AS earlier ON earlier.`ID` = prerequisite.`ID` AND earlier.`NextQuestID` = 29678
JOIN `quest_template_addon` AS shu ON shu.`ID` = 29678 AND shu.`PrevQuestID` = 29666
JOIN `quest_template` AS offering ON offering.`ID` = shu.`ID` AND offering.`StartItem` = 73791
WHERE prerequisite.`ID` IN (29666, 29677)
  AND EXISTS (SELECT 1 FROM `quest_template_addon` WHERE `ID` = 29666 AND `NextQuestID` = 29678)
  AND EXISTS (SELECT 1 FROM `quest_template_addon` WHERE `ID` = 29677 AND `NextQuestID` = 29678)
  AND NOT EXISTS (
      SELECT 1 FROM `conditions` WHERE `SourceTypeOrReferenceId` = 19 AND `SourceEntry` = 29678
  );
