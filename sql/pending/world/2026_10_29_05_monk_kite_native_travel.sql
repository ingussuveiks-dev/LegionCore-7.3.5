-- CODE-COUPLED: keep outside updater inputs until the registered hit handler is promoted.
SET @monk_kite_concat := @@SESSION.group_concat_max_len;
SET SESSION group_concat_max_len=GREATEST(@@SESSION.group_concat_max_len,1048576);
START TRANSACTION;
SET @monk_kite_names := (SELECT COUNT(*) FROM spell_script_names WHERE spell_id=202475);
SET @monk_kite_bound := (SELECT COUNT(*) FROM spell_script_names WHERE spell_id=202475 AND ScriptName='spell_monk_kite_travel');
SET @monk_kite_rows := (SELECT COUNT(*) FROM smart_scripts WHERE entryorguid=102531 AND source_type=0);
SET @monk_kite_hash := (SELECT SHA2(GROUP_CONCAT(CONCAT_WS('|',COALESCE(CAST(`entryorguid` AS CHAR),'NULL'),COALESCE(CAST(`source_type` AS CHAR),'NULL'),COALESCE(CAST(`id` AS CHAR),'NULL'),COALESCE(CAST(`link` AS CHAR),'NULL'),COALESCE(CAST(`Difficulties` AS CHAR),'NULL'),COALESCE(CAST(`event_type` AS CHAR),'NULL'),COALESCE(CAST(`event_phase_mask` AS CHAR),'NULL'),COALESCE(CAST(`event_chance` AS CHAR),'NULL'),COALESCE(CAST(`event_flags` AS CHAR),'NULL'),COALESCE(CAST(`event_param1` AS CHAR),'NULL'),COALESCE(CAST(`event_param2` AS CHAR),'NULL'),COALESCE(CAST(`event_param3` AS CHAR),'NULL'),COALESCE(CAST(`event_param4` AS CHAR),'NULL'),COALESCE(CAST(`event_param5` AS CHAR),'NULL'),COALESCE(CAST(`action_type` AS CHAR),'NULL'),COALESCE(CAST(`action_param1` AS CHAR),'NULL'),COALESCE(CAST(`action_param2` AS CHAR),'NULL'),COALESCE(CAST(`action_param3` AS CHAR),'NULL'),COALESCE(CAST(`action_param4` AS CHAR),'NULL'),COALESCE(CAST(`action_param5` AS CHAR),'NULL'),COALESCE(CAST(`action_param6` AS CHAR),'NULL'),COALESCE(CAST(`target_type` AS CHAR),'NULL'),COALESCE(CAST(`target_param1` AS CHAR),'NULL'),COALESCE(CAST(`target_param2` AS CHAR),'NULL'),COALESCE(CAST(`target_param3` AS CHAR),'NULL'),COALESCE(CAST(`target_param4` AS CHAR),'NULL'),COALESCE(CAST(`target_x` AS CHAR),'NULL'),COALESCE(CAST(`target_y` AS CHAR),'NULL'),COALESCE(CAST(`target_z` AS CHAR),'NULL'),COALESCE(CAST(`target_o` AS CHAR),'NULL'),COALESCE(CAST(`comment` AS CHAR),'NULL')) ORDER BY `entryorguid`,`source_type`,`id`,`link`,`Difficulties`,`event_type`,`event_phase_mask`,`event_chance`,`event_flags`,`event_param1`,`event_param2`,`event_param3`,`event_param4`,`event_param5`,`action_type`,`action_param1`,`action_param2`,`action_param3`,`action_param4`,`action_param5`,`action_param6`,`target_type`,`target_param1`,`target_param2`,`target_param3`,`target_param4`,`target_x`,`target_y`,`target_z`,`target_o`,`comment` SEPARATOR '\n'),256) FROM smart_scripts WHERE entryorguid=102531 AND source_type=0);
SET @monk_kite_safe := (SELECT COUNT(*) FROM creature_template WHERE entry=102531 AND AIName='SmartAI' AND ScriptName='' AND VehicleId=0 AND (npcflag&16777216)<>0)=1
 AND (SELECT COUNT(*) FROM npc_spellclick_spells WHERE npc_entry=102531)=1
 AND (SELECT COUNT(*) FROM npc_spellclick_spells WHERE npc_entry=102531 AND spell_id=202475 AND cast_flags=1 AND user_type=0 AND add_npc_flag=1)=1
 AND (SELECT COUNT(*) FROM npc_spellclick_spells WHERE spell_id=202475)=1
 AND (SELECT COUNT(*) FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceEntry=102531)=0
 AND (SELECT COUNT(*) FROM spell_target_position WHERE id=202481)=1
 AND (SELECT SHA2(CONCAT_WS('|',id,target_map,target_position_x,target_position_y,target_position_z,target_orientation),256) FROM spell_target_position WHERE id=202481)='ad01ace085cf1b1113b429ed76e464182b0c5abf499372f522730c0c53951be3'
 AND ((@monk_kite_rows=2 AND @monk_kite_hash='815d69e19d5afa2f3d7c41da6b361ca3a952355f84f2fc0c291fb3137f69dcb0' AND @monk_kite_names=0)
 OR (@monk_kite_rows=0 AND @monk_kite_names=1 AND @monk_kite_bound=1));
SET @monk_kite_check := IF(COALESCE(@monk_kite_safe,0),'DO 0','SIGNAL SQLSTATE ''45000'' SET MESSAGE_TEXT=''Monk kite native click/actions/binding differ; review before promotion''');
PREPARE monk_kite_check FROM @monk_kite_check;EXECUTE monk_kite_check;DEALLOCATE PREPARE monk_kite_check;
DELETE FROM smart_scripts WHERE @monk_kite_safe AND entryorguid=102531 AND source_type=0;
INSERT INTO spell_script_names(spell_id,ScriptName) SELECT 202475,'spell_monk_kite_travel'
 WHERE @monk_kite_safe AND @monk_kite_names=0;
COMMIT;
SET SESSION group_concat_max_len=@monk_kite_concat;
