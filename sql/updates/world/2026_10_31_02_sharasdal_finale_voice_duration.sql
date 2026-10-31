-- Matching26972 native voices and original line offsets:1906 requires23788ms,1923 requires23727ms.
-- Native Conversation sums creature durations and adds5000ms. Change only first-actor budgets.
-- Preserve all actors/line IDs, offsets and unknown payloads. This does not repair callers/defeat/pickup timing.
START TRANSACTION;
SET @sharasdal_voice_lines=(SELECT COUNT(*) FROM conversation_data WHERE entry IN(1906,1923))=9 AND (SELECT COUNT(*) FROM conversation_data WHERE (entry,id,textId,unk1,unk2,idx) IN((1906,4028,0,3269427312,0,0),(1906,4029,6267,3269427312,1,1),(1906,4030,10932,3269427312,2,2),(1906,4031,15983,3269427312,0,3),(1906,4032,20866,3269427312,2,4),(1923,4066,0,0,0,0),(1923,4068,12950,0,1,1),(1923,4069,14700,0,0,2),(1923,4071,19050,0,2,3)))=9;
SET @sharasdal_voice_old=(SELECT COUNT(*) FROM conversation_creature WHERE entry IN(1906,1923))=6 AND (SELECT COUNT(*) FROM conversation_creature WHERE (entry,id,creatureId,creatureGuid,unk1,unk2,duration) IN((1906,0,102826,743508,0,0,2811),(1906,1,104856,743605,0,0,2811),(1906,2,105298,743508,0,0,2811),(1923,0,102826,7195931,0,0,4600),(1923,1,105298,7195931,0,0,4600),(1923,2,104853,7195931,0,0,4600)))=6;
SET @sharasdal_voice_applied=(SELECT COUNT(*) FROM conversation_creature WHERE entry IN(1906,1923))=6 AND (SELECT COUNT(*) FROM conversation_creature WHERE (entry,id,creatureId,creatureGuid,unk1,unk2,duration) IN((1906,0,102826,743508,0,0,18166),(1906,1,104856,743605,0,0,2811),(1906,2,105298,743508,0,0,2811),(1923,0,102826,7195931,0,0,14527),(1923,1,105298,7195931,0,0,4600),(1923,2,104853,7195931,0,0,4600)))=6;
SET @sharasdal_voice_safe=@sharasdal_voice_lines AND (@sharasdal_voice_old OR @sharasdal_voice_applied) AND (SELECT COUNT(*) FROM conversation_actor WHERE entry IN(1906,1923))=0;
SET @sharasdal_voice_guard=IF(COALESCE(@sharasdal_voice_safe,0),'DO 0','SIGNAL SQLSTATE ''45000'' SET MESSAGE_TEXT=''Sharasdal finale voice context differs; review before updating''');
PREPARE sharasdal_voice_guard FROM @sharasdal_voice_guard;EXECUTE sharasdal_voice_guard;DEALLOCATE PREPARE sharasdal_voice_guard;
UPDATE conversation_creature SET duration=CASE entry WHEN 1906 THEN 18166 WHEN 1923 THEN 14527 END WHERE entry IN(1906,1923) AND id=0 AND @sharasdal_voice_safe AND @sharasdal_voice_old;
COMMIT;
