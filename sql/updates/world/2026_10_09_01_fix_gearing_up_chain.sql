-- Garrison Campaign: Out of the Chains -> Gearing Up -> Seeking the Truth.
-- Native 7.3.5.26972 QuestLineXQuest.db2: line 115, rows 1615/1616/1617.
-- Corroborated by https://www.wowhead.com/quest=34315/gearing-up
-- This is Shadow Hunter Bwu'ja's Draenor quest, not the custom boost tutorial.
-- Repair the prerequisite, incoming successor prerequisite and reward handoff
-- together; do not overwrite a differently customized chain.
UPDATE `quest_template` q
JOIN `quest_template_addon` a ON a.`ID` = q.`ID`
SET q.`RewardNextQuest` = 34316, a.`PrevQuestID` = 34314, a.`NextQuestID` = 34316
WHERE q.`ID` = 34315 AND q.`RewardNextQuest` = 34315
  AND a.`PrevQuestID` = 34315 AND a.`NextQuestID` = 34315;
