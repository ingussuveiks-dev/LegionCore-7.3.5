-- No Wings Required and To Weather the Storm are parallel after A Grapple
-- a Day. Match the already-present Horde prerequisite on the Alliance side.
UPDATE quest_template_addon SET PrevQuestID=38312
WHERE ID=38405 AND PrevQuestID=0;

-- QuestData adds both NextQuestID predecessors, which are OR alternatives
-- unless their negative ExclusiveGroup requires every member to be rewarded.
-- Impalement Insurance must not bypass either infiltration or equipment.
UPDATE quest_template_addon SET ExclusiveGroup=-38318
WHERE ID IN(38318,38405) AND ExclusiveGroup=0 AND NextQuestID=38410;
UPDATE quest_template_addon SET ExclusiveGroup=-38613
WHERE ID IN(38613,38614) AND ExclusiveGroup=0 AND NextQuestID=38615;
