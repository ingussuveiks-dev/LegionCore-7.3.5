-- Bloodtotem opening: the three Witchwood tasks are parallel after Oakin.
UPDATE quest_template_addon SET NextQuestID=39372 WHERE ID=38912;
UPDATE quest_template_addon SET PrevQuestID=38912,NextQuestID=39374,ExclusiveGroup=-39372
WHERE ID IN (39372,39373,39873);
UPDATE quest_template_addon SET PrevQuestID=39372 WHERE ID=39374;
-- A bonus objective must not provide an alternative late-story prerequisite.
UPDATE quest_template_addon SET NextQuestID=0 WHERE ID=39371 AND NextQuestID=39372;

-- TDB735 build 25549: 50 feathers. Runtime ItemSparse hotfix raises MaxCount
-- to 50; the native stack size remains 30. Login reconciles existing saves.
UPDATE quest_objectives SET Amount=50,Flags2=1
WHERE ID=279309 AND QuestID=39373 AND Type=1 AND ObjectID=128380;
