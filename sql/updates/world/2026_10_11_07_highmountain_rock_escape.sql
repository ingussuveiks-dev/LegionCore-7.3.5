-- 39381: retain native talk credit/visual, but make the delayed escape owner-only and retryable.
UPDATE creature_template SET AIName='',ScriptName='npc_rock_escape_navarrogg' WHERE entry=96286;
DELETE FROM smart_scripts WHERE entryorguid=96286 AND source_type=0 AND event_type=62 AND event_param1=18673 AND event_param2=0;
DELETE FROM smart_scripts WHERE entryorguid=9628600 AND source_type=9;
-- A completed-but-unrewarded quest may still be inside the cave after a failed teleport/relog.
DELETE FROM conditions WHERE SourceTypeOrReferenceId=15 AND SourceGroup=18673 AND SourceEntry=0;
INSERT INTO conditions (SourceTypeOrReferenceId,SourceGroup,SourceEntry,SourceId,ElseGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2,ConditionValue3,NegativeCondition,ErrorTextId,ScriptName,Comment) VALUES
(15,18673,0,0,0,9,0,39381,0,0,0,0,'','Rock escape: quest incomplete'),
(15,18673,0,0,1,28,0,39381,0,0,0,0,'','Rock escape: completed quest can retry before reward');
-- TDB735: all three refuge quests point to 39426. Restore the parallel offer and all-three reward gate.
UPDATE quest_template_addon SET PrevQuestID=39381,NextQuestID=39426,ExclusiveGroup=-39391 WHERE ID IN(39391,39425,39588);
UPDATE quest_template SET RewardNextQuest=39426 WHERE ID IN(39391,39425,39588);
UPDATE quest_template_addon SET PrevQuestID=39391 WHERE ID=39426;
