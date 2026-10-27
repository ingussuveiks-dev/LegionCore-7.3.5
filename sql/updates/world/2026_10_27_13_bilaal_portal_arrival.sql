-- CODE-COUPLED: keep outside updater until matching instance1583 EXE is promoted.
-- Only the genuine104546 waypoint arrival arms map-owned248553 creation retry.
-- Optional223908 presentation no longer gates portal/flags/fly cleanup.
SET @bilaal_portal_previous_group_concat := @@SESSION.group_concat_max_len;
SET SESSION group_concat_max_len=GREATEST(@@SESSION.group_concat_max_len,1048576);
START TRANSACTION;
SET @bilaal_portal_graph := (SELECT SHA2(GROUP_CONCAT(CONCAT_WS('|',entryorguid,source_type,id,link,Difficulties,event_type,event_phase_mask,event_chance,event_flags,event_param1,event_param2,event_param3,event_param4,event_param5,action_type,action_param1,action_param2,action_param3,action_param4,action_param5,action_param6,target_type,target_param1,target_param2,target_param3,target_param4,target_x,target_y,target_z,target_o,comment) ORDER BY source_type,id SEPARATOR '\n'),256) FROM smart_scripts WHERE entryorguid=104546);
SET @bilaal_portal_safe := (SELECT COUNT(*) FROM smart_scripts WHERE entryorguid=104546)=11
 AND @bilaal_portal_graph IN('d013c5d9516aef3d3e4d5f6a82b82bdcee1ad231f5052c9f5a1b3dfb3499e0b5','83d195ce948f80d71ddbc824ab23a32b5c11fbb6ec7e476202e2701da1fcd571')
 AND (SELECT COUNT(*) FROM creature_template WHERE entry=104546 AND AIName='SmartAI' AND ScriptName='')=1
 AND (SELECT COUNT(*) FROM instance_template WHERE map=1583 AND parent=0 AND script='instance_priest_mage_nexus_art' AND allowMount=1 AND bonusChance=20)=1
 AND (SELECT COUNT(*) FROM waypoints WHERE entry=104546)=1
 AND (SELECT COUNT(*) FROM waypoints WHERE entry=104546 AND pointid=1 AND ABS(position_x-4204.35)<0.001 AND ABS(position_y-7090.24)<0.001 AND ABS(position_z-268.198)<0.001)=1
 AND (SELECT COUNT(*) FROM gameobject_template WHERE entry=248553 AND type=22 AND displayId=8111 AND Data0=207901 AND Data3=1 AND AIName='' AND ScriptName='')=1
 AND (SELECT COUNT(*) FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceEntry=104546)=2
 AND (SELECT COUNT(*) FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceEntry=104546 AND SourceGroup=2 AND SourceId=0 AND ConditionTypeOrReference=44 AND ConditionTarget=1 AND ConditionValue2=5 AND ConditionValue3=0 AND NegativeCondition=0 AND ErrorTextId=0 AND ScriptName='' AND ((ElseGroup=0 AND ConditionValue1=1101) OR(ElseGroup=1 AND ConditionValue1=1065)))=2;
SET @bilaal_portal_guard := IF(@bilaal_portal_safe,'DO 0',
 'SIGNAL SQLSTATE ''45000'' SET MESSAGE_TEXT=''Azuregos native graph/path/instance/portal/conditions differ; review before promotion''');
PREPARE bilaal_portal_check FROM @bilaal_portal_guard;
EXECUTE bilaal_portal_check;
DEALLOCATE PREPARE bilaal_portal_check;
UPDATE smart_scripts SET link=0 WHERE @bilaal_portal_safe AND entryorguid=104546 AND source_type=0 AND id IN(1,3);
DELETE FROM smart_scripts WHERE @bilaal_portal_safe AND entryorguid=104546 AND source_type=0 AND id=4;
INSERT INTO smart_scripts(entryorguid,source_type,id,link,Difficulties,event_type,event_phase_mask,event_chance,event_flags,event_param1,event_param2,event_param3,event_param4,event_param5,action_type,action_param1,action_param2,action_param3,action_param4,action_param5,action_param6,target_type,target_param1,target_param2,target_param3,target_param4,target_x,target_y,target_z,target_o,comment) SELECT 104546,0,7,5,'',40,0,100,1,1,104546,0,0,0,64,104546,0,0,0,0,0,1,0,0,0,0,0,0,0,0,'WP Reached - Commit earned Azuregos portal arrival' WHERE @bilaal_portal_safe AND NOT EXISTS(SELECT 1 FROM smart_scripts WHERE entryorguid=104546 AND source_type=0 AND id=7);
COMMIT;
SET SESSION group_concat_max_len=@bilaal_portal_previous_group_concat;
