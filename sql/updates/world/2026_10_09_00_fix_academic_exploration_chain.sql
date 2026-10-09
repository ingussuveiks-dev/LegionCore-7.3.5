-- Legion Archaeology: Academic Exploration -> Tried and True -> The Keys to Success.
-- A NextQuestID self-link becomes a prerequisite on itself in LoadQuests(),
-- making the rotating first quest unavailable to a new character.
-- Confirmed by 41184.PrevQuestID and the documented quest storyline:
-- https://www.wowhead.com/quest=41183/academic-exploration
-- Preserve archaeology rank, event 165, questgivers and all other prerequisites.
UPDATE `quest_template_addon` SET `NextQuestID` = 41184
WHERE `ID` = 41183 AND `PrevQuestID` = 0 AND `NextQuestID` = 41183;
