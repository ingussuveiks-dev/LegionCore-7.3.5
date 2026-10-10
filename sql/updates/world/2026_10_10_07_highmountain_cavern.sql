-- Native 7.3.5 objective and alternate enemy credit.
UPDATE quest_objectives SET Flags2=1 WHERE ID=279636 AND QuestID=39488 AND ObjectID=128393;
UPDATE creature_template_wdb SET KillCredit1=95866 WHERE Entry=95916 AND KillCredit1 IN(0,95866);

-- The two cavern tasks are offered together; both turn-ins unlock Gelmogg.
UPDATE quest_template_addon SET PrevQuestID=39661,NextQuestID=39487,ExclusiveGroup=-39488
WHERE ID IN(39488,39489);
-- NextQuestID also creates a server prerequisite. The old spray shortcut
-- must not bypass the two cavern turn-ins (retain native RewardNextQuest).
UPDATE quest_template_addon SET NextQuestID=0 WHERE ID=39277 AND NextQuestID=39487;
INSERT IGNORE INTO creature_queststarter (id,quest) VALUES (96520,39489),(96520,39487),(96520,39498);

-- Existing Siphoning Crystal loot was orphaned from its chest template.
-- Retain the legitimate creature drops; do not invent crystal spawn positions.
UPDATE gameobject_template SET Data1=243639 WHERE entry=243639 AND type=3 AND Data1 IN(0,243639);

-- Do not skip the transition when a hit takes Gelmogg below 40%.
UPDATE smart_scripts SET event_param1=0 WHERE entryorguid=95881 AND source_type=0 AND id=3
    AND event_type=2 AND event_param2=50 AND action_type=80 AND action_param1=9588100;
-- Gelmogg becomes 95882 at 12 seconds. Dargrul's signal at 15.5 seconds
-- formerly searched for the obsolete 95881 entry, never starting phase 2.
UPDATE smart_scripts SET target_param1=95882 WHERE entryorguid=9946000 AND source_type=9 AND id=6
    AND action_type=45 AND action_param1=1 AND action_param2=1 AND target_type=11 AND target_param1 IN(95881,95882);
