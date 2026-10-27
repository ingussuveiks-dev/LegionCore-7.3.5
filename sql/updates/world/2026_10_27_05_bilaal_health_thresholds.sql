-- Both matching7.3.5 Nexus-Prince Bilaal variants: three health thresholds.
-- Native209863 is a15s self aura; do not refresh it on every eligible AI tick.
-- Code-coupled: SMARTCAST_RETRY_HEALTH_THRESHOLD256 must be in promoted EXE.
START TRANSACTION;
SET @bilaal_graph := (SELECT SHA2(GROUP_CONCAT(CONCAT_WS('|',entryorguid,source_type,id,link,Difficulties,event_type,event_phase_mask,event_chance,IF(id IN(2,3,4),0,event_flags),event_param1,event_param2,event_param3,event_param4,event_param5,action_type,action_param1,IF(id IN(2,3,4),0,action_param2),action_param3,action_param4,action_param5,action_param6,target_type,target_param1,target_param2,target_param3,target_param4,target_x,target_y,target_z,target_o,comment)
 ORDER BY entryorguid,id SEPARATOR '\n'),256) FROM smart_scripts WHERE entryorguid IN(104502,106708) AND source_type=0);
SET @bilaal_safe := @bilaal_graph='76c9aaf82ab0138dfd090653728b7418cfd7e098c5f97042c8c2511045b89668'
 AND (SELECT COUNT(*) FROM smart_scripts WHERE entryorguid IN(104502,106708) AND source_type=0)=14
 AND (SELECT COUNT(*) FROM creature_template WHERE entry IN(104502,106708) AND AIName='SmartAI' AND ScriptName='')=2
 AND NOT EXISTS(SELECT 1 FROM smart_scripts WHERE entryorguid IN(104502,106708) AND source_type=0 AND id IN(2,3,4)
 AND NOT((event_flags=0 AND action_param2=0) OR(event_flags=1 AND action_param2=256)))
 AND NOT EXISTS(SELECT 1 FROM smart_scripts WHERE action_type IN(11,85,210) AND(action_param2&256)<>0
 AND NOT(entryorguid IN(104502,106708) AND source_type=0 AND id IN(2,3,4) AND action_param2=256));
SET @bilaal_guard := IF(@bilaal_safe,'DO 0',
 'SIGNAL SQLSTATE ''45000'' SET MESSAGE_TEXT=''Bilaal native combat graph or retry flag differs; review before promotion''');
PREPARE bilaal_check FROM @bilaal_guard;
EXECUTE bilaal_check;
DEALLOCATE PREPARE bilaal_check;
UPDATE smart_scripts SET event_flags=1,action_param2=256
 WHERE @bilaal_safe AND entryorguid IN(104502,106708) AND source_type=0 AND id IN(2,3,4);
COMMIT;
