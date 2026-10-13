-- The return to Dreadscar follows the introduction and one artifact branch.
UPDATE quest_template_addon SET PrevQuestID=40729 WHERE ID=40731 AND PrevQuestID=0;
-- These three return-to-Calydus alternatives were attached to Dark Whispers
-- itself, making the first Demonology artifact require an existing artifact.
DELETE FROM conditions WHERE SourceTypeOrReferenceId=19 AND SourceGroup=0
 AND SourceEntry=42125 AND SourceId=0 AND ConditionTypeOrReference=8
 AND ConditionTarget=0 AND NegativeCondition=0 AND ConditionValue2=0 AND ConditionValue3=0
 AND ((ElseGroup=0 AND ConditionValue1=42125) OR (ElseGroup=1 AND ConditionValue1=40712)
 OR (ElseGroup=2 AND ConditionValue1=43254));
INSERT IGNORE INTO conditions
 (SourceTypeOrReferenceId,SourceGroup,SourceEntry,SourceId,ElseGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2,ConditionValue3,NegativeCondition,Comment)
VALUES
 (19,0,40731,0,0,8,0,40712,0,0,0,'Dreadscar - Affliction artifact returned'),
 (19,0,40731,0,1,8,0,41156,0,0,0,'Dreadscar - alternate Affliction return'),
 (19,0,40731,0,2,8,0,42125,0,0,0,'Dreadscar - Demonology artifact returned'),
 (19,0,40731,0,3,8,0,43254,0,0,0,'Dreadscar - Destruction artifact returned');

-- ACTION_INVOKER is the killing unit, which can be the warlock's demon.
-- This solo scenario must credit nearby players for a pet's killing blow too.
UPDATE smart_scripts SET target_type=18,target_param1=100,
 comment='Jagganoth death - scenario credit to nearby players, including pet kills'
 WHERE entryorguid=101468 AND source_type=0 AND id=4 AND event_type=6
 AND action_type=205 AND action_param1=92 AND action_param2=48186 AND target_type=7;
UPDATE smart_scripts SET target_type=18,target_param1=100
 WHERE entryorguid=101468 AND source_type=0 AND id=7 AND event_type=61
 AND action_type=11 AND action_param1=200125 AND target_type=7;

-- Native ScenarioStep/CriteriaTree/Criteria: 998 step 0=kill (48186),
-- step 1=own heart 132880, step 2=place heart (48182) and audience (48187).
INSERT IGNORE INTO conditions
 (SourceTypeOrReferenceId,SourceGroup,SourceEntry,SourceId,ElseGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2,ConditionValue3,NegativeCondition,Comment)
VALUES
 (18,101468,200095,0,0,9,0,40731,0,0,0,'Jagganoth heart - quest active'),
 (18,101468,200095,0,0,44,0,998,1,0,0,'Jagganoth heart - after defeat'),
 (18,101468,200095,0,0,36,1,0,0,0,1,'Jagganoth heart - corpse only'),
 (18,101470,200095,0,0,9,0,40731,0,0,0,'Heart - quest active'),
 (18,101470,200095,0,0,44,0,998,1,0,0,'Heart - collection step'),
 (22,1,246148,1,0,9,0,40731,0,0,0,'Altar - quest active'),
 (22,1,246148,1,0,44,0,998,2,0,0,'Altar - final scenario step'),
 (22,1,246148,1,0,2,0,132880,1,0,0,'Altar - must carry the heart');

-- Re-entry through Calydus reuses the original scenario selector and teleport.
-- Both the visible option and its handler are restricted to an active quest
-- in Dalaran (world map or the Underbelly map), not the finished class hall.
INSERT IGNORE INTO gossip_menu_option
 (MenuID,OptionID,OptionNpc,OptionText,OptionBroadcastTextID,ActionMenuID,ActionPoiID,BoxCoded,BoxMoney,BoxText,BoxBroadcastTextID,VerifiedBuild)
VALUES (19326,2,0,'Return to Dreadscar Rift.',0,0,0,0,0,'',0,0);
INSERT IGNORE INTO smart_scripts
 (entryorguid,source_type,id,link,event_type,event_chance,event_param1,event_param2,action_type,action_param1,target_type,comment)
VALUES (101097,0,5,3,62,100,19326,2,204,998,7,'Calydus - return to unfinished Dreadscar scenario');
INSERT IGNORE INTO conditions
 (SourceTypeOrReferenceId,SourceGroup,SourceEntry,SourceId,ElseGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2,ConditionValue3,NegativeCondition,Comment)
VALUES
 (15,19326,2,0,0,9,0,40731,0,0,0,'Dreadscar return - active quest'),
 (15,19326,2,0,0,22,0,1502,0,0,0,'Dreadscar return - Underbelly'),
 (15,19326,2,0,1,9,0,40731,0,0,0,'Dreadscar return - active quest'),
 (15,19326,2,0,1,22,0,1220,0,0,0,'Dreadscar return - Dalaran'),
 (22,6,101097,0,0,9,0,40731,0,0,0,'Dreadscar return handler - active quest'),
 (22,6,101097,0,0,22,0,1502,0,0,0,'Dreadscar return handler - Underbelly'),
 (22,6,101097,0,1,9,0,40731,0,0,0,'Dreadscar return handler - active quest'),
 (22,6,101097,0,1,22,0,1220,0,0,0,'Dreadscar return handler - Dalaran');

-- A fresh instance can start at the kill step while a returning player still
-- carries the previously earned heart. Once Jagganoth is defeated, restore
-- only OWN_ITEM progress from that real item; never grant an item or a kill.
INSERT IGNORE INTO smart_scripts
 (entryorguid,source_type,id,event_type,event_chance,event_param1,event_param2,event_param3,event_param4,action_type,action_param1,action_param2,action_param3,target_type,comment)
VALUES (101470,0,2,10,100,1,30,1000,1000,205,36,132880,1,7,'Heart - recover existing inventory progress after re-entry');
INSERT IGNORE INTO conditions
 (SourceTypeOrReferenceId,SourceGroup,SourceEntry,SourceId,ElseGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2,ConditionValue3,NegativeCondition,Comment)
VALUES
 (22,3,101470,0,0,9,0,40731,0,0,0,'Heart recovery - active quest'),
 (22,3,101470,0,0,44,0,998,1,0,0,'Heart recovery - boss already defeated'),
 (22,3,101470,0,0,2,0,132880,1,0,0,'Heart recovery - item already owned');
