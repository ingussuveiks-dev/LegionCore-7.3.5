-- Matching26972 ConversationLine chain3961->3962->8701->8702->8703->0.
-- Keep original0/6400 offsets and two actor payloads. Later offsets are derived
-- from hashed native enUS voice lengths plus native AdditionalDuration; they
-- are conservative scheduling, not a claim of captured Blizzard timestamps.
-- Actual Conversation sums actor durations and adds5000ms:42816+5000=47816.
-- This data fix does not repair the upper intro caller/admission/NPC lifetime.
START TRANSACTION;
SET @sharasdal_upper_lines_old=(SELECT COUNT(*) FROM conversation_data WHERE entry=1868)=2 AND (SELECT COUNT(*) FROM conversation_data WHERE (entry,id,textId,unk1,unk2,idx) IN((1868,3961,0,0,0,0),(1868,3962,6400,0,1,1)))=2;
SET @sharasdal_upper_lines_applied=(SELECT COUNT(*) FROM conversation_data WHERE entry=1868)=5 AND (SELECT COUNT(*) FROM conversation_data WHERE (entry,id,textId,unk1,unk2,idx) IN((1868,3961,0,0,0,0),(1868,3962,6400,0,1,1),(1868,8701,14733,0,1,2),(1868,8702,28691,0,0,3),(1868,8703,30866,0,1,4)))=5;
SET @sharasdal_upper_actors_old=(SELECT COUNT(*) FROM conversation_creature WHERE entry=1868)=2 AND (SELECT COUNT(*) FROM conversation_creature WHERE (entry,id,creatureId,creatureGuid,unk1,unk2,duration) IN((1868,0,104853,7195931,0,0,7800),(1868,1,102826,7195931,0,0,7800)))=2;
SET @sharasdal_upper_actors_applied=(SELECT COUNT(*) FROM conversation_creature WHERE entry=1868)=2 AND (SELECT COUNT(*) FROM conversation_creature WHERE (entry,id,creatureId,creatureGuid,unk1,unk2,duration) IN((1868,0,104853,7195931,0,0,7800),(1868,1,102826,7195931,0,0,35016)))=2;
SET @sharasdal_upper_old=@sharasdal_upper_lines_old AND @sharasdal_upper_actors_old;
SET @sharasdal_upper_applied=@sharasdal_upper_lines_applied AND @sharasdal_upper_actors_applied;
SET @sharasdal_upper_safe=(@sharasdal_upper_old OR @sharasdal_upper_applied) AND (SELECT COUNT(*) FROM conversation_actor WHERE entry=1868)=0;
SET @sharasdal_upper_guard=IF(COALESCE(@sharasdal_upper_safe,0),'DO 0','SIGNAL SQLSTATE ''45000'' SET MESSAGE_TEXT=''Sharasdal upper intro context differs; review before updating''');
PREPARE sharasdal_upper_guard FROM @sharasdal_upper_guard;EXECUTE sharasdal_upper_guard;DEALLOCATE PREPARE sharasdal_upper_guard;
INSERT INTO conversation_data(entry,id,textId,unk1,unk2,idx)
SELECT 1868,8701,14733,0,1,2 WHERE @sharasdal_upper_safe AND @sharasdal_upper_old
UNION ALL SELECT 1868,8702,28691,0,0,3 WHERE @sharasdal_upper_safe AND @sharasdal_upper_old
UNION ALL SELECT 1868,8703,30866,0,1,4 WHERE @sharasdal_upper_safe AND @sharasdal_upper_old;
UPDATE conversation_creature SET duration=35016 WHERE entry=1868 AND id=1 AND @sharasdal_upper_safe AND @sharasdal_upper_old;
COMMIT;
