-- Code-coupled: native Smart action50 must support NO_CREATOR flag4 first.
-- Keep original lifetime0/summoner/phase; the instance owns the claimed chest
-- after its visual104985 source executes its existing immediate despawn.
START TRANSACTION;
SET @lights_chest_safe := EXISTS(SELECT 1 FROM creature_template WHERE entry=104985 AND AIName='SmartAI' AND ScriptName='')
 AND EXISTS(SELECT 1 FROM gameobject_template WHERE entry=248572 AND type=3 AND Data0=1691
 AND Data1=248572 AND AIName='SmartGameObjectAI' AND ScriptName='go_artifact_acquisition')
 AND (SELECT COUNT(*) FROM smart_scripts WHERE entryorguid=104985 AND source_type IN(0,9))=7
 AND (SELECT COUNT(*) FROM smart_scripts WHERE entryorguid=104985 AND (
 (source_type=0 AND event_flags=1 AND target_type=1 AND
 ((id=0 AND link=1 AND event_type=73 AND action_type=11 AND action_param1=207956)
 OR(id=1 AND link=2 AND event_type=61 AND action_type=80 AND action_param1=104985 AND action_param2=2)
 OR(id=2 AND link=0 AND event_type=61 AND action_type=81 AND action_param1=0)))
 OR(source_type=9 AND event_type=61 AND link=0 AND event_flags=1 AND
 ((id=0 AND event_param1=30000 AND event_param2=30000 AND action_type=85 AND action_param1=207996 AND target_type=7)
 OR(id=1 AND action_type=205 AND action_param1=92 AND action_param2=49811 AND target_type=7)
 OR(id=2 AND action_type=50 AND action_param1=248572 AND action_param2=0 AND action_param3 IN(0,4)
 AND target_type=8 AND ABS(target_x-1117.78)<0.001 AND ABS(target_y-1048.03)<0.001
 AND ABS(target_z-310.663)<0.001 AND ABS(target_o-2.09636)<0.001)
 OR(id=3 AND action_type=41 AND action_param1=0 AND target_type=1)))))=7;
SET @lights_chest_guard := IF(@lights_chest_safe,'DO 0',
 'SIGNAL SQLSTATE ''45000'' SET MESSAGE_TEXT=''Lights Wrath chest custom/missing native graph; review before promotion''');
PREPARE lights_chest_check FROM @lights_chest_guard;
EXECUTE lights_chest_check;
DEALLOCATE PREPARE lights_chest_check;
UPDATE smart_scripts SET action_param3=4 WHERE @lights_chest_safe
 AND entryorguid=104985 AND source_type=9 AND id=2 AND action_type=50 AND action_param1=248572;
COMMIT;
