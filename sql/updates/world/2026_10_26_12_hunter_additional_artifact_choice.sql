-- Hunter Emmarel review interaction, matching choice240/native198430.
-- Code-coupled: publish only with the display/response condition consumers.
-- Preserve hidden selections for abandoned-route retry; actual acquisition
-- finales, not inventory ownership, exclude finished routes. Level102 follows44043.
-- Refuse all managed changes when an unexpected custom row owns a target slot.
SET @hunter_offer_safe :=
 NOT EXISTS (SELECT 1 FROM conditions WHERE
 ((SourceTypeOrReferenceId=15 AND SourceGroup IN(19115,20227) AND SourceEntry=0 AND SourceId=0)
 OR (SourceTypeOrReferenceId=32 AND SourceGroup=0 AND SourceEntry=240 AND SourceId=0)
 OR (SourceTypeOrReferenceId=33 AND SourceGroup=0 AND SourceEntry IN(504,505,506) AND SourceId=0))
 AND COALESCE(Comment,'') NOT LIKE 'Hunter artifact choice - %'
 AND NOT (SourceTypeOrReferenceId=15 AND SourceGroup=19115 AND SourceEntry=0 AND SourceId=0
 AND ElseGroup=0 AND ConditionTypeOrReference=8 AND ConditionTarget=0 AND ConditionValue1=40618
 AND ConditionValue2=0 AND ConditionValue3=0 AND NegativeCondition=0))
 AND NOT EXISTS (SELECT 1 FROM gossip_menu_option WHERE MenuID=20227 AND OptionID=0
 AND NOT (OptionNpc=0 AND OptionBroadcastTextID=103645 AND ActionMenuID=0 AND ActionPoiID=0
 AND BoxCoded=0 AND BoxMoney=0 AND BoxBroadcastTextID=0))
 AND NOT EXISTS (SELECT 1 FROM smart_scripts WHERE source_type=0
 AND ((entryorguid=107317 AND id IN(0,1)) OR (entryorguid=107973 AND id=1))
 AND COALESCE(comment,'') NOT LIKE 'Hunter artifact choice - %')
 AND (SELECT COUNT(*) FROM creature_template WHERE entry IN(107317,107973)
 AND gossip_menu_id=20227 AND AIName='SmartAI' AND ScriptName='' AND (npcflag&2)<>0)=2;
UPDATE creature_template SET npcflag=npcflag|1
 WHERE @hunter_offer_safe AND entry IN(107317,107973) AND gossip_menu_id=20227 AND AIName='SmartAI' AND ScriptName='';
INSERT INTO gossip_menu_option
 (MenuID,OptionID,OptionNpc,OptionText,OptionBroadcastTextID)
 SELECT 20227,0,0,'I would like to review weapons we might pursue.',103645
 WHERE @hunter_offer_safe AND NOT EXISTS(SELECT 1 FROM gossip_menu_option WHERE MenuID=20227 AND OptionID=0);
DELETE FROM smart_scripts WHERE @hunter_offer_safe AND source_type=0
 AND ((entryorguid=107317 AND id IN(0,1)) OR (entryorguid=107973 AND id=1))
 AND comment LIKE 'Hunter artifact choice - %';
INSERT INTO smart_scripts
 (entryorguid,source_type,id,event_type,event_param1,event_param2,action_type,action_param1,target_type,comment)
 SELECT 107317,0,0,62,20227,0,85,198430,7,'Hunter artifact choice - review native weapons' WHERE @hunter_offer_safe
 UNION ALL SELECT 107973,0,1,62,20227,0,85,198430,7,'Hunter artifact choice - review native weapons' WHERE @hunter_offer_safe
 UNION ALL SELECT 107317,0,1,19,44043,0,85,198430,7,'Hunter artifact choice - Continuing the Legend acceptance' WHERE @hunter_offer_safe;
DELETE FROM conditions WHERE @hunter_offer_safe AND
 ((SourceTypeOrReferenceId=15 AND SourceGroup IN(19115,20227) AND SourceEntry=0 AND SourceId=0)
 OR (SourceTypeOrReferenceId=32 AND SourceGroup=0 AND SourceEntry=240 AND SourceId=0)
 OR (SourceTypeOrReferenceId=33 AND SourceGroup=0 AND SourceEntry IN(504,505,506) AND SourceId=0));
INSERT INTO conditions
 (SourceTypeOrReferenceId,SourceGroup,SourceEntry,SourceId,ElseGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2,ConditionValue3,NegativeCondition,Comment)
SELECT * FROM (
 SELECT 32 AS src,0 AS grp,240 AS ent,0 AS sid,0 AS branch,15 AS kind,0 AS tgt,4 AS v1,0 AS v2,0 AS v3,0 AS neg,'Hunter artifact choice - Hunter only' AS note
 UNION ALL
 SELECT 32,0,240,0,0,27,0,98,3,0,0,'Hunter artifact choice - native first selection level'
 UNION ALL
 SELECT 32,0,240,0,0,9,0,40618,0,0,0,'Hunter artifact choice - Weapons of Legend active or finished'
 UNION ALL
 SELECT 32,0,240,0,1,15,0,4,0,0,0,'Hunter artifact choice - Hunter only'
 UNION ALL
 SELECT 32,0,240,0,1,27,0,98,3,0,0,'Hunter artifact choice - native first selection level'
 UNION ALL
 SELECT 32,0,240,0,1,28,0,40618,0,0,0,'Hunter artifact choice - Weapons of Legend active or finished'
 UNION ALL
 SELECT 32,0,240,0,2,15,0,4,0,0,0,'Hunter artifact choice - Hunter only'
 UNION ALL
 SELECT 32,0,240,0,2,27,0,98,3,0,0,'Hunter artifact choice - native first selection level'
 UNION ALL
 SELECT 32,0,240,0,2,8,0,40618,0,0,0,'Hunter artifact choice - Weapons of Legend active or finished'
 UNION ALL
 SELECT 33,0,504,0,0,15,0,4,0,0,0,'Hunter artifact choice - Hunter only'
 UNION ALL
 SELECT 33,0,504,0,0,27,0,98,3,0,0,'Hunter artifact choice - native first selection level'
 UNION ALL
 SELECT 33,0,504,0,0,8,0,42185,0,0,1,'Hunter artifact choice - route not rewarded'
 UNION ALL
 SELECT 33,0,504,0,0,28,0,42185,0,0,1,'Hunter artifact choice - route not awaiting turn-in'
 UNION ALL
 SELECT 33,0,504,0,0,14,0,40621,0,0,0,'Hunter artifact choice - no initial selection'
 UNION ALL
 SELECT 33,0,504,0,0,14,0,40619,0,0,0,'Hunter artifact choice - no initial selection'
 UNION ALL
 SELECT 33,0,504,0,0,14,0,40620,0,0,0,'Hunter artifact choice - no initial selection'
 UNION ALL
 SELECT 15,19115,0,0,0,15,0,4,0,0,0,'Hunter artifact choice - Hunter only'
 UNION ALL
 SELECT 15,19115,0,0,0,27,0,98,3,0,0,'Hunter artifact choice - native first selection level'
 UNION ALL
 SELECT 15,19115,0,0,0,8,0,42185,0,0,1,'Hunter artifact choice - route not rewarded'
 UNION ALL
 SELECT 15,19115,0,0,0,28,0,42185,0,0,1,'Hunter artifact choice - route not awaiting turn-in'
 UNION ALL
 SELECT 15,19115,0,0,0,8,0,40618,0,0,0,'Hunter artifact choice - introduction rewarded'
 UNION ALL
 SELECT 15,19115,0,0,0,14,0,40621,0,0,0,'Hunter artifact choice - no initial selection'
 UNION ALL
 SELECT 15,19115,0,0,0,14,0,40619,0,0,0,'Hunter artifact choice - no initial selection'
 UNION ALL
 SELECT 15,19115,0,0,0,14,0,40620,0,0,0,'Hunter artifact choice - no initial selection'
 UNION ALL
 SELECT 15,20227,0,0,0,15,0,4,0,0,0,'Hunter artifact choice - Hunter only'
 UNION ALL
 SELECT 15,20227,0,0,0,27,0,98,3,0,0,'Hunter artifact choice - native first selection level'
 UNION ALL
 SELECT 15,20227,0,0,0,8,0,42185,0,0,1,'Hunter artifact choice - route not rewarded'
 UNION ALL
 SELECT 15,20227,0,0,0,28,0,42185,0,0,1,'Hunter artifact choice - route not awaiting turn-in'
 UNION ALL
 SELECT 15,20227,0,0,0,8,0,40618,0,0,0,'Hunter artifact choice - introduction rewarded'
 UNION ALL
 SELECT 15,20227,0,0,0,14,0,40621,0,0,0,'Hunter artifact choice - no initial selection'
 UNION ALL
 SELECT 15,20227,0,0,0,14,0,40619,0,0,0,'Hunter artifact choice - no initial selection'
 UNION ALL
 SELECT 15,20227,0,0,0,14,0,40620,0,0,0,'Hunter artifact choice - no initial selection'
 UNION ALL
 SELECT 33,0,504,0,1,15,0,4,0,0,0,'Hunter artifact choice - Hunter only'
 UNION ALL
 SELECT 33,0,504,0,1,27,0,98,3,0,0,'Hunter artifact choice - native first selection level'
 UNION ALL
 SELECT 33,0,504,0,1,8,0,42185,0,0,1,'Hunter artifact choice - route not rewarded'
 UNION ALL
 SELECT 33,0,504,0,1,28,0,42185,0,0,1,'Hunter artifact choice - route not awaiting turn-in'
 UNION ALL
 SELECT 33,0,504,0,1,8,0,40621,0,0,0,'Hunter artifact choice - retry selected unfinished route'
 UNION ALL
 SELECT 15,19115,0,0,1,15,0,4,0,0,0,'Hunter artifact choice - Hunter only'
 UNION ALL
 SELECT 15,19115,0,0,1,27,0,98,3,0,0,'Hunter artifact choice - native first selection level'
 UNION ALL
 SELECT 15,19115,0,0,1,8,0,42185,0,0,1,'Hunter artifact choice - route not rewarded'
 UNION ALL
 SELECT 15,19115,0,0,1,28,0,42185,0,0,1,'Hunter artifact choice - route not awaiting turn-in'
 UNION ALL
 SELECT 15,19115,0,0,1,8,0,40618,0,0,0,'Hunter artifact choice - introduction rewarded'
 UNION ALL
 SELECT 15,19115,0,0,1,8,0,40621,0,0,0,'Hunter artifact choice - retry selected unfinished route'
 UNION ALL
 SELECT 15,20227,0,0,1,15,0,4,0,0,0,'Hunter artifact choice - Hunter only'
 UNION ALL
 SELECT 15,20227,0,0,1,27,0,98,3,0,0,'Hunter artifact choice - native first selection level'
 UNION ALL
 SELECT 15,20227,0,0,1,8,0,42185,0,0,1,'Hunter artifact choice - route not rewarded'
 UNION ALL
 SELECT 15,20227,0,0,1,28,0,42185,0,0,1,'Hunter artifact choice - route not awaiting turn-in'
 UNION ALL
 SELECT 15,20227,0,0,1,8,0,40618,0,0,0,'Hunter artifact choice - introduction rewarded'
 UNION ALL
 SELECT 15,20227,0,0,1,8,0,40621,0,0,0,'Hunter artifact choice - retry selected unfinished route'
 UNION ALL
 SELECT 33,0,504,0,3,15,0,4,0,0,0,'Hunter artifact choice - Hunter only'
 UNION ALL
 SELECT 33,0,504,0,3,27,0,98,3,0,0,'Hunter artifact choice - native first selection level'
 UNION ALL
 SELECT 33,0,504,0,3,8,0,42185,0,0,1,'Hunter artifact choice - route not rewarded'
 UNION ALL
 SELECT 33,0,504,0,3,28,0,42185,0,0,1,'Hunter artifact choice - route not awaiting turn-in'
 UNION ALL
 SELECT 33,0,504,0,3,27,0,102,3,0,0,'Hunter artifact choice - additional selection level'
 UNION ALL
 SELECT 33,0,504,0,3,8,0,40385,0,0,0,'Hunter artifact choice - previous acquisition rewarded'
 UNION ALL
 SELECT 15,19115,0,0,3,15,0,4,0,0,0,'Hunter artifact choice - Hunter only'
 UNION ALL
 SELECT 15,19115,0,0,3,27,0,98,3,0,0,'Hunter artifact choice - native first selection level'
 UNION ALL
 SELECT 15,19115,0,0,3,8,0,42185,0,0,1,'Hunter artifact choice - route not rewarded'
 UNION ALL
 SELECT 15,19115,0,0,3,28,0,42185,0,0,1,'Hunter artifact choice - route not awaiting turn-in'
 UNION ALL
 SELECT 15,19115,0,0,3,8,0,40618,0,0,0,'Hunter artifact choice - introduction rewarded'
 UNION ALL
 SELECT 15,19115,0,0,3,27,0,102,3,0,0,'Hunter artifact choice - additional selection level'
 UNION ALL
 SELECT 15,19115,0,0,3,8,0,40385,0,0,0,'Hunter artifact choice - previous acquisition rewarded'
 UNION ALL
 SELECT 15,20227,0,0,3,15,0,4,0,0,0,'Hunter artifact choice - Hunter only'
 UNION ALL
 SELECT 15,20227,0,0,3,27,0,98,3,0,0,'Hunter artifact choice - native first selection level'
 UNION ALL
 SELECT 15,20227,0,0,3,8,0,42185,0,0,1,'Hunter artifact choice - route not rewarded'
 UNION ALL
 SELECT 15,20227,0,0,3,28,0,42185,0,0,1,'Hunter artifact choice - route not awaiting turn-in'
 UNION ALL
 SELECT 15,20227,0,0,3,8,0,40618,0,0,0,'Hunter artifact choice - introduction rewarded'
 UNION ALL
 SELECT 15,20227,0,0,3,27,0,102,3,0,0,'Hunter artifact choice - additional selection level'
 UNION ALL
 SELECT 15,20227,0,0,3,8,0,40385,0,0,0,'Hunter artifact choice - previous acquisition rewarded'
 UNION ALL
 SELECT 33,0,504,0,4,15,0,4,0,0,0,'Hunter artifact choice - Hunter only'
 UNION ALL
 SELECT 33,0,504,0,4,27,0,98,3,0,0,'Hunter artifact choice - native first selection level'
 UNION ALL
 SELECT 33,0,504,0,4,8,0,42185,0,0,1,'Hunter artifact choice - route not rewarded'
 UNION ALL
 SELECT 33,0,504,0,4,28,0,42185,0,0,1,'Hunter artifact choice - route not awaiting turn-in'
 UNION ALL
 SELECT 33,0,504,0,4,27,0,102,3,0,0,'Hunter artifact choice - additional selection level'
 UNION ALL
 SELECT 33,0,504,0,4,8,0,40419,0,0,0,'Hunter artifact choice - previous acquisition rewarded'
 UNION ALL
 SELECT 15,19115,0,0,4,15,0,4,0,0,0,'Hunter artifact choice - Hunter only'
 UNION ALL
 SELECT 15,19115,0,0,4,27,0,98,3,0,0,'Hunter artifact choice - native first selection level'
 UNION ALL
 SELECT 15,19115,0,0,4,8,0,42185,0,0,1,'Hunter artifact choice - route not rewarded'
 UNION ALL
 SELECT 15,19115,0,0,4,28,0,42185,0,0,1,'Hunter artifact choice - route not awaiting turn-in'
 UNION ALL
 SELECT 15,19115,0,0,4,8,0,40618,0,0,0,'Hunter artifact choice - introduction rewarded'
 UNION ALL
 SELECT 15,19115,0,0,4,27,0,102,3,0,0,'Hunter artifact choice - additional selection level'
 UNION ALL
 SELECT 15,19115,0,0,4,8,0,40419,0,0,0,'Hunter artifact choice - previous acquisition rewarded'
 UNION ALL
 SELECT 15,20227,0,0,4,15,0,4,0,0,0,'Hunter artifact choice - Hunter only'
 UNION ALL
 SELECT 15,20227,0,0,4,27,0,98,3,0,0,'Hunter artifact choice - native first selection level'
 UNION ALL
 SELECT 15,20227,0,0,4,8,0,42185,0,0,1,'Hunter artifact choice - route not rewarded'
 UNION ALL
 SELECT 15,20227,0,0,4,28,0,42185,0,0,1,'Hunter artifact choice - route not awaiting turn-in'
 UNION ALL
 SELECT 15,20227,0,0,4,8,0,40618,0,0,0,'Hunter artifact choice - introduction rewarded'
 UNION ALL
 SELECT 15,20227,0,0,4,27,0,102,3,0,0,'Hunter artifact choice - additional selection level'
 UNION ALL
 SELECT 15,20227,0,0,4,8,0,40419,0,0,0,'Hunter artifact choice - previous acquisition rewarded'
 UNION ALL
 SELECT 33,0,505,0,0,15,0,4,0,0,0,'Hunter artifact choice - Hunter only'
 UNION ALL
 SELECT 33,0,505,0,0,27,0,98,3,0,0,'Hunter artifact choice - native first selection level'
 UNION ALL
 SELECT 33,0,505,0,0,8,0,40385,0,0,1,'Hunter artifact choice - route not rewarded'
 UNION ALL
 SELECT 33,0,505,0,0,28,0,40385,0,0,1,'Hunter artifact choice - route not awaiting turn-in'
 UNION ALL
 SELECT 33,0,505,0,0,14,0,40621,0,0,0,'Hunter artifact choice - no initial selection'
 UNION ALL
 SELECT 33,0,505,0,0,14,0,40619,0,0,0,'Hunter artifact choice - no initial selection'
 UNION ALL
 SELECT 33,0,505,0,0,14,0,40620,0,0,0,'Hunter artifact choice - no initial selection'
 UNION ALL
 SELECT 15,19115,0,0,5,15,0,4,0,0,0,'Hunter artifact choice - Hunter only'
 UNION ALL
 SELECT 15,19115,0,0,5,27,0,98,3,0,0,'Hunter artifact choice - native first selection level'
 UNION ALL
 SELECT 15,19115,0,0,5,8,0,40385,0,0,1,'Hunter artifact choice - route not rewarded'
 UNION ALL
 SELECT 15,19115,0,0,5,28,0,40385,0,0,1,'Hunter artifact choice - route not awaiting turn-in'
 UNION ALL
 SELECT 15,19115,0,0,5,8,0,40618,0,0,0,'Hunter artifact choice - introduction rewarded'
 UNION ALL
 SELECT 15,19115,0,0,5,14,0,40621,0,0,0,'Hunter artifact choice - no initial selection'
 UNION ALL
 SELECT 15,19115,0,0,5,14,0,40619,0,0,0,'Hunter artifact choice - no initial selection'
 UNION ALL
 SELECT 15,19115,0,0,5,14,0,40620,0,0,0,'Hunter artifact choice - no initial selection'
 UNION ALL
 SELECT 15,20227,0,0,5,15,0,4,0,0,0,'Hunter artifact choice - Hunter only'
 UNION ALL
 SELECT 15,20227,0,0,5,27,0,98,3,0,0,'Hunter artifact choice - native first selection level'
 UNION ALL
 SELECT 15,20227,0,0,5,8,0,40385,0,0,1,'Hunter artifact choice - route not rewarded'
 UNION ALL
 SELECT 15,20227,0,0,5,28,0,40385,0,0,1,'Hunter artifact choice - route not awaiting turn-in'
 UNION ALL
 SELECT 15,20227,0,0,5,8,0,40618,0,0,0,'Hunter artifact choice - introduction rewarded'
 UNION ALL
 SELECT 15,20227,0,0,5,14,0,40621,0,0,0,'Hunter artifact choice - no initial selection'
 UNION ALL
 SELECT 15,20227,0,0,5,14,0,40619,0,0,0,'Hunter artifact choice - no initial selection'
 UNION ALL
 SELECT 15,20227,0,0,5,14,0,40620,0,0,0,'Hunter artifact choice - no initial selection'
 UNION ALL
 SELECT 33,0,505,0,1,15,0,4,0,0,0,'Hunter artifact choice - Hunter only'
 UNION ALL
 SELECT 33,0,505,0,1,27,0,98,3,0,0,'Hunter artifact choice - native first selection level'
 UNION ALL
 SELECT 33,0,505,0,1,8,0,40385,0,0,1,'Hunter artifact choice - route not rewarded'
 UNION ALL
 SELECT 33,0,505,0,1,28,0,40385,0,0,1,'Hunter artifact choice - route not awaiting turn-in'
 UNION ALL
 SELECT 33,0,505,0,1,8,0,40619,0,0,0,'Hunter artifact choice - retry selected unfinished route'
 UNION ALL
 SELECT 15,19115,0,0,6,15,0,4,0,0,0,'Hunter artifact choice - Hunter only'
 UNION ALL
 SELECT 15,19115,0,0,6,27,0,98,3,0,0,'Hunter artifact choice - native first selection level'
 UNION ALL
 SELECT 15,19115,0,0,6,8,0,40385,0,0,1,'Hunter artifact choice - route not rewarded'
 UNION ALL
 SELECT 15,19115,0,0,6,28,0,40385,0,0,1,'Hunter artifact choice - route not awaiting turn-in'
 UNION ALL
 SELECT 15,19115,0,0,6,8,0,40618,0,0,0,'Hunter artifact choice - introduction rewarded'
 UNION ALL
 SELECT 15,19115,0,0,6,8,0,40619,0,0,0,'Hunter artifact choice - retry selected unfinished route'
 UNION ALL
 SELECT 15,20227,0,0,6,15,0,4,0,0,0,'Hunter artifact choice - Hunter only'
 UNION ALL
 SELECT 15,20227,0,0,6,27,0,98,3,0,0,'Hunter artifact choice - native first selection level'
 UNION ALL
 SELECT 15,20227,0,0,6,8,0,40385,0,0,1,'Hunter artifact choice - route not rewarded'
 UNION ALL
 SELECT 15,20227,0,0,6,28,0,40385,0,0,1,'Hunter artifact choice - route not awaiting turn-in'
 UNION ALL
 SELECT 15,20227,0,0,6,8,0,40618,0,0,0,'Hunter artifact choice - introduction rewarded'
 UNION ALL
 SELECT 15,20227,0,0,6,8,0,40619,0,0,0,'Hunter artifact choice - retry selected unfinished route'
 UNION ALL
 SELECT 33,0,505,0,2,15,0,4,0,0,0,'Hunter artifact choice - Hunter only'
 UNION ALL
 SELECT 33,0,505,0,2,27,0,98,3,0,0,'Hunter artifact choice - native first selection level'
 UNION ALL
 SELECT 33,0,505,0,2,8,0,40385,0,0,1,'Hunter artifact choice - route not rewarded'
 UNION ALL
 SELECT 33,0,505,0,2,28,0,40385,0,0,1,'Hunter artifact choice - route not awaiting turn-in'
 UNION ALL
 SELECT 33,0,505,0,2,27,0,102,3,0,0,'Hunter artifact choice - additional selection level'
 UNION ALL
 SELECT 33,0,505,0,2,8,0,42185,0,0,0,'Hunter artifact choice - previous acquisition rewarded'
 UNION ALL
 SELECT 15,19115,0,0,7,15,0,4,0,0,0,'Hunter artifact choice - Hunter only'
 UNION ALL
 SELECT 15,19115,0,0,7,27,0,98,3,0,0,'Hunter artifact choice - native first selection level'
 UNION ALL
 SELECT 15,19115,0,0,7,8,0,40385,0,0,1,'Hunter artifact choice - route not rewarded'
 UNION ALL
 SELECT 15,19115,0,0,7,28,0,40385,0,0,1,'Hunter artifact choice - route not awaiting turn-in'
 UNION ALL
 SELECT 15,19115,0,0,7,8,0,40618,0,0,0,'Hunter artifact choice - introduction rewarded'
 UNION ALL
 SELECT 15,19115,0,0,7,27,0,102,3,0,0,'Hunter artifact choice - additional selection level'
 UNION ALL
 SELECT 15,19115,0,0,7,8,0,42185,0,0,0,'Hunter artifact choice - previous acquisition rewarded'
 UNION ALL
 SELECT 15,20227,0,0,7,15,0,4,0,0,0,'Hunter artifact choice - Hunter only'
 UNION ALL
 SELECT 15,20227,0,0,7,27,0,98,3,0,0,'Hunter artifact choice - native first selection level'
 UNION ALL
 SELECT 15,20227,0,0,7,8,0,40385,0,0,1,'Hunter artifact choice - route not rewarded'
 UNION ALL
 SELECT 15,20227,0,0,7,28,0,40385,0,0,1,'Hunter artifact choice - route not awaiting turn-in'
 UNION ALL
 SELECT 15,20227,0,0,7,8,0,40618,0,0,0,'Hunter artifact choice - introduction rewarded'
 UNION ALL
 SELECT 15,20227,0,0,7,27,0,102,3,0,0,'Hunter artifact choice - additional selection level'
 UNION ALL
 SELECT 15,20227,0,0,7,8,0,42185,0,0,0,'Hunter artifact choice - previous acquisition rewarded'
 UNION ALL
 SELECT 33,0,505,0,4,15,0,4,0,0,0,'Hunter artifact choice - Hunter only'
 UNION ALL
 SELECT 33,0,505,0,4,27,0,98,3,0,0,'Hunter artifact choice - native first selection level'
 UNION ALL
 SELECT 33,0,505,0,4,8,0,40385,0,0,1,'Hunter artifact choice - route not rewarded'
 UNION ALL
 SELECT 33,0,505,0,4,28,0,40385,0,0,1,'Hunter artifact choice - route not awaiting turn-in'
 UNION ALL
 SELECT 33,0,505,0,4,27,0,102,3,0,0,'Hunter artifact choice - additional selection level'
 UNION ALL
 SELECT 33,0,505,0,4,8,0,40419,0,0,0,'Hunter artifact choice - previous acquisition rewarded'
 UNION ALL
 SELECT 15,19115,0,0,9,15,0,4,0,0,0,'Hunter artifact choice - Hunter only'
 UNION ALL
 SELECT 15,19115,0,0,9,27,0,98,3,0,0,'Hunter artifact choice - native first selection level'
 UNION ALL
 SELECT 15,19115,0,0,9,8,0,40385,0,0,1,'Hunter artifact choice - route not rewarded'
 UNION ALL
 SELECT 15,19115,0,0,9,28,0,40385,0,0,1,'Hunter artifact choice - route not awaiting turn-in'
 UNION ALL
 SELECT 15,19115,0,0,9,8,0,40618,0,0,0,'Hunter artifact choice - introduction rewarded'
 UNION ALL
 SELECT 15,19115,0,0,9,27,0,102,3,0,0,'Hunter artifact choice - additional selection level'
 UNION ALL
 SELECT 15,19115,0,0,9,8,0,40419,0,0,0,'Hunter artifact choice - previous acquisition rewarded'
 UNION ALL
 SELECT 15,20227,0,0,9,15,0,4,0,0,0,'Hunter artifact choice - Hunter only'
 UNION ALL
 SELECT 15,20227,0,0,9,27,0,98,3,0,0,'Hunter artifact choice - native first selection level'
 UNION ALL
 SELECT 15,20227,0,0,9,8,0,40385,0,0,1,'Hunter artifact choice - route not rewarded'
 UNION ALL
 SELECT 15,20227,0,0,9,28,0,40385,0,0,1,'Hunter artifact choice - route not awaiting turn-in'
 UNION ALL
 SELECT 15,20227,0,0,9,8,0,40618,0,0,0,'Hunter artifact choice - introduction rewarded'
 UNION ALL
 SELECT 15,20227,0,0,9,27,0,102,3,0,0,'Hunter artifact choice - additional selection level'
 UNION ALL
 SELECT 15,20227,0,0,9,8,0,40419,0,0,0,'Hunter artifact choice - previous acquisition rewarded'
 UNION ALL
 SELECT 33,0,506,0,0,15,0,4,0,0,0,'Hunter artifact choice - Hunter only'
 UNION ALL
 SELECT 33,0,506,0,0,27,0,98,3,0,0,'Hunter artifact choice - native first selection level'
 UNION ALL
 SELECT 33,0,506,0,0,8,0,40419,0,0,1,'Hunter artifact choice - route not rewarded'
 UNION ALL
 SELECT 33,0,506,0,0,28,0,40419,0,0,1,'Hunter artifact choice - route not awaiting turn-in'
 UNION ALL
 SELECT 33,0,506,0,0,14,0,40621,0,0,0,'Hunter artifact choice - no initial selection'
 UNION ALL
 SELECT 33,0,506,0,0,14,0,40619,0,0,0,'Hunter artifact choice - no initial selection'
 UNION ALL
 SELECT 33,0,506,0,0,14,0,40620,0,0,0,'Hunter artifact choice - no initial selection'
 UNION ALL
 SELECT 15,19115,0,0,10,15,0,4,0,0,0,'Hunter artifact choice - Hunter only'
 UNION ALL
 SELECT 15,19115,0,0,10,27,0,98,3,0,0,'Hunter artifact choice - native first selection level'
 UNION ALL
 SELECT 15,19115,0,0,10,8,0,40419,0,0,1,'Hunter artifact choice - route not rewarded'
 UNION ALL
 SELECT 15,19115,0,0,10,28,0,40419,0,0,1,'Hunter artifact choice - route not awaiting turn-in'
 UNION ALL
 SELECT 15,19115,0,0,10,8,0,40618,0,0,0,'Hunter artifact choice - introduction rewarded'
 UNION ALL
 SELECT 15,19115,0,0,10,14,0,40621,0,0,0,'Hunter artifact choice - no initial selection'
 UNION ALL
 SELECT 15,19115,0,0,10,14,0,40619,0,0,0,'Hunter artifact choice - no initial selection'
 UNION ALL
 SELECT 15,19115,0,0,10,14,0,40620,0,0,0,'Hunter artifact choice - no initial selection'
 UNION ALL
 SELECT 15,20227,0,0,10,15,0,4,0,0,0,'Hunter artifact choice - Hunter only'
 UNION ALL
 SELECT 15,20227,0,0,10,27,0,98,3,0,0,'Hunter artifact choice - native first selection level'
 UNION ALL
 SELECT 15,20227,0,0,10,8,0,40419,0,0,1,'Hunter artifact choice - route not rewarded'
 UNION ALL
 SELECT 15,20227,0,0,10,28,0,40419,0,0,1,'Hunter artifact choice - route not awaiting turn-in'
 UNION ALL
 SELECT 15,20227,0,0,10,8,0,40618,0,0,0,'Hunter artifact choice - introduction rewarded'
 UNION ALL
 SELECT 15,20227,0,0,10,14,0,40621,0,0,0,'Hunter artifact choice - no initial selection'
 UNION ALL
 SELECT 15,20227,0,0,10,14,0,40619,0,0,0,'Hunter artifact choice - no initial selection'
 UNION ALL
 SELECT 15,20227,0,0,10,14,0,40620,0,0,0,'Hunter artifact choice - no initial selection'
 UNION ALL
 SELECT 33,0,506,0,1,15,0,4,0,0,0,'Hunter artifact choice - Hunter only'
 UNION ALL
 SELECT 33,0,506,0,1,27,0,98,3,0,0,'Hunter artifact choice - native first selection level'
 UNION ALL
 SELECT 33,0,506,0,1,8,0,40419,0,0,1,'Hunter artifact choice - route not rewarded'
 UNION ALL
 SELECT 33,0,506,0,1,28,0,40419,0,0,1,'Hunter artifact choice - route not awaiting turn-in'
 UNION ALL
 SELECT 33,0,506,0,1,8,0,40620,0,0,0,'Hunter artifact choice - retry selected unfinished route'
 UNION ALL
 SELECT 15,19115,0,0,11,15,0,4,0,0,0,'Hunter artifact choice - Hunter only'
 UNION ALL
 SELECT 15,19115,0,0,11,27,0,98,3,0,0,'Hunter artifact choice - native first selection level'
 UNION ALL
 SELECT 15,19115,0,0,11,8,0,40419,0,0,1,'Hunter artifact choice - route not rewarded'
 UNION ALL
 SELECT 15,19115,0,0,11,28,0,40419,0,0,1,'Hunter artifact choice - route not awaiting turn-in'
 UNION ALL
 SELECT 15,19115,0,0,11,8,0,40618,0,0,0,'Hunter artifact choice - introduction rewarded'
 UNION ALL
 SELECT 15,19115,0,0,11,8,0,40620,0,0,0,'Hunter artifact choice - retry selected unfinished route'
 UNION ALL
 SELECT 15,20227,0,0,11,15,0,4,0,0,0,'Hunter artifact choice - Hunter only'
 UNION ALL
 SELECT 15,20227,0,0,11,27,0,98,3,0,0,'Hunter artifact choice - native first selection level'
 UNION ALL
 SELECT 15,20227,0,0,11,8,0,40419,0,0,1,'Hunter artifact choice - route not rewarded'
 UNION ALL
 SELECT 15,20227,0,0,11,28,0,40419,0,0,1,'Hunter artifact choice - route not awaiting turn-in'
 UNION ALL
 SELECT 15,20227,0,0,11,8,0,40618,0,0,0,'Hunter artifact choice - introduction rewarded'
 UNION ALL
 SELECT 15,20227,0,0,11,8,0,40620,0,0,0,'Hunter artifact choice - retry selected unfinished route'
 UNION ALL
 SELECT 33,0,506,0,2,15,0,4,0,0,0,'Hunter artifact choice - Hunter only'
 UNION ALL
 SELECT 33,0,506,0,2,27,0,98,3,0,0,'Hunter artifact choice - native first selection level'
 UNION ALL
 SELECT 33,0,506,0,2,8,0,40419,0,0,1,'Hunter artifact choice - route not rewarded'
 UNION ALL
 SELECT 33,0,506,0,2,28,0,40419,0,0,1,'Hunter artifact choice - route not awaiting turn-in'
 UNION ALL
 SELECT 33,0,506,0,2,27,0,102,3,0,0,'Hunter artifact choice - additional selection level'
 UNION ALL
 SELECT 33,0,506,0,2,8,0,42185,0,0,0,'Hunter artifact choice - previous acquisition rewarded'
 UNION ALL
 SELECT 15,19115,0,0,12,15,0,4,0,0,0,'Hunter artifact choice - Hunter only'
 UNION ALL
 SELECT 15,19115,0,0,12,27,0,98,3,0,0,'Hunter artifact choice - native first selection level'
 UNION ALL
 SELECT 15,19115,0,0,12,8,0,40419,0,0,1,'Hunter artifact choice - route not rewarded'
 UNION ALL
 SELECT 15,19115,0,0,12,28,0,40419,0,0,1,'Hunter artifact choice - route not awaiting turn-in'
 UNION ALL
 SELECT 15,19115,0,0,12,8,0,40618,0,0,0,'Hunter artifact choice - introduction rewarded'
 UNION ALL
 SELECT 15,19115,0,0,12,27,0,102,3,0,0,'Hunter artifact choice - additional selection level'
 UNION ALL
 SELECT 15,19115,0,0,12,8,0,42185,0,0,0,'Hunter artifact choice - previous acquisition rewarded'
 UNION ALL
 SELECT 15,20227,0,0,12,15,0,4,0,0,0,'Hunter artifact choice - Hunter only'
 UNION ALL
 SELECT 15,20227,0,0,12,27,0,98,3,0,0,'Hunter artifact choice - native first selection level'
 UNION ALL
 SELECT 15,20227,0,0,12,8,0,40419,0,0,1,'Hunter artifact choice - route not rewarded'
 UNION ALL
 SELECT 15,20227,0,0,12,28,0,40419,0,0,1,'Hunter artifact choice - route not awaiting turn-in'
 UNION ALL
 SELECT 15,20227,0,0,12,8,0,40618,0,0,0,'Hunter artifact choice - introduction rewarded'
 UNION ALL
 SELECT 15,20227,0,0,12,27,0,102,3,0,0,'Hunter artifact choice - additional selection level'
 UNION ALL
 SELECT 15,20227,0,0,12,8,0,42185,0,0,0,'Hunter artifact choice - previous acquisition rewarded'
 UNION ALL
 SELECT 33,0,506,0,3,15,0,4,0,0,0,'Hunter artifact choice - Hunter only'
 UNION ALL
 SELECT 33,0,506,0,3,27,0,98,3,0,0,'Hunter artifact choice - native first selection level'
 UNION ALL
 SELECT 33,0,506,0,3,8,0,40419,0,0,1,'Hunter artifact choice - route not rewarded'
 UNION ALL
 SELECT 33,0,506,0,3,28,0,40419,0,0,1,'Hunter artifact choice - route not awaiting turn-in'
 UNION ALL
 SELECT 33,0,506,0,3,27,0,102,3,0,0,'Hunter artifact choice - additional selection level'
 UNION ALL
 SELECT 33,0,506,0,3,8,0,40385,0,0,0,'Hunter artifact choice - previous acquisition rewarded'
 UNION ALL
 SELECT 15,19115,0,0,13,15,0,4,0,0,0,'Hunter artifact choice - Hunter only'
 UNION ALL
 SELECT 15,19115,0,0,13,27,0,98,3,0,0,'Hunter artifact choice - native first selection level'
 UNION ALL
 SELECT 15,19115,0,0,13,8,0,40419,0,0,1,'Hunter artifact choice - route not rewarded'
 UNION ALL
 SELECT 15,19115,0,0,13,28,0,40419,0,0,1,'Hunter artifact choice - route not awaiting turn-in'
 UNION ALL
 SELECT 15,19115,0,0,13,8,0,40618,0,0,0,'Hunter artifact choice - introduction rewarded'
 UNION ALL
 SELECT 15,19115,0,0,13,27,0,102,3,0,0,'Hunter artifact choice - additional selection level'
 UNION ALL
 SELECT 15,19115,0,0,13,8,0,40385,0,0,0,'Hunter artifact choice - previous acquisition rewarded'
 UNION ALL
 SELECT 15,20227,0,0,13,15,0,4,0,0,0,'Hunter artifact choice - Hunter only'
 UNION ALL
 SELECT 15,20227,0,0,13,27,0,98,3,0,0,'Hunter artifact choice - native first selection level'
 UNION ALL
 SELECT 15,20227,0,0,13,8,0,40419,0,0,1,'Hunter artifact choice - route not rewarded'
 UNION ALL
 SELECT 15,20227,0,0,13,28,0,40419,0,0,1,'Hunter artifact choice - route not awaiting turn-in'
 UNION ALL
 SELECT 15,20227,0,0,13,8,0,40618,0,0,0,'Hunter artifact choice - introduction rewarded'
 UNION ALL
 SELECT 15,20227,0,0,13,27,0,102,3,0,0,'Hunter artifact choice - additional selection level'
 UNION ALL
 SELECT 15,20227,0,0,13,8,0,40385,0,0,0,'Hunter artifact choice - previous acquisition rewarded'
) AS hunter_conditions WHERE @hunter_offer_safe;
