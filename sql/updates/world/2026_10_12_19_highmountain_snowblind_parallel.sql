-- Snowblind Mesa: accept all three battle quests after the journey turn-in.
-- Preserve the existing requirement to reward all three before evacuation,
-- using the core's negative exclusive group instead of an artificial serial chain.
UPDATE `quest_template_addon`
SET `PrevQuestID`=38915, `NextQuestID`=42088, `ExclusiveGroup`=-39776
WHERE `ID` IN (39776,39777,39862) AND `ExclusiveGroup` IN (0,-39776)
  AND `PrevQuestID` IN (38915,39776,39777) AND `NextQuestID` IN (39777,39862,42088);
UPDATE `quest_template_addon` SET `PrevQuestID`=39776
WHERE `ID`=42088 AND `PrevQuestID` IN (39862,39776);

-- Keep AggressorAI for every other spell; only fix the native ANY-target attacks.
UPDATE `creature_template` SET `ScriptName`='npc_highmountain_mightstone'
WHERE `entry` IN (96690,96691,97546) AND `AIName`=''
  AND `ScriptName` IN ('','npc_highmountain_mightstone');
