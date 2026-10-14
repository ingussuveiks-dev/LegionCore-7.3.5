-- Both faction-specific Thorignir handoffs follow the shared Watchers blessing.
-- Horde's old Seal Your Fate prerequisite exposed this handoff before the trials.
-- Keep the native race masks, Havi/Lorna/Cullen relations and empty breadcrumb objectives.
UPDATE quest_template_addon SET PrevQuestID=39597
WHERE ID=38611 AND PrevQuestID=38459 AND NextQuestID=38612;
