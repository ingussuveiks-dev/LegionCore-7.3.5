START TRANSACTION;
-- Only the Warrior crypt controller must not inherit the Priest defense dome.
-- Keep native stage6 instant204394 reveal and the existing6000ms103226 summon.
INSERT INTO creature_addon (guid,path_id,mount,bytes1,bytes2,emote,auras)
SELECT c.guid,a.path_id,a.mount,a.bytes1,a.bytes2,a.emote,''
FROM creature c JOIN creature_template_addon a ON a.entry=c.id
JOIN creature_template t ON t.entry=c.id
WHERE c.guid=369687 AND c.id=101461 AND c.map=1539 AND c.PhaseId='5739'
 AND ABS(c.position_x-1962.7)<0.01 AND ABS(c.position_y-2336.35)<0.01 AND ABS(c.position_z-80.3631)<0.01
 AND t.AIName='SmartAI' AND t.ScriptName=''
 AND a.path_id=0 AND a.mount=0 AND a.bytes1=0 AND a.bytes2=0 AND a.emote=0 AND a.auras='200984'
 AND NOT EXISTS(SELECT 1 FROM creature_addon ca WHERE ca.guid=c.guid)
 AND EXISTS(SELECT 1 FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceGroup=2 AND SourceEntry=101461
     AND SourceId=0 AND ElseGroup=0 AND ConditionTypeOrReference=44 AND ConditionTarget=1
     AND ConditionValue1=1037 AND ConditionValue2=5 AND ConditionValue3=0 AND NegativeCondition=0 AND ScriptName='')
 AND EXISTS(SELECT 1 FROM smart_scripts WHERE entryorguid=101461 AND source_type=9 AND id=0 AND link=0
     AND event_type=0 AND event_flags=1 AND event_param1=0 AND event_param2=0
     AND action_type=11 AND action_param1=204394 AND action_param2=0 AND target_type=1)
 AND EXISTS(SELECT 1 FROM smart_scripts WHERE entryorguid=101461 AND source_type=9 AND id=1 AND link=0
     AND event_type=0 AND event_flags=1 AND event_param1=6000 AND event_param2=6000
     AND action_type=12 AND action_param1=103226 AND action_param2=7 AND target_type=8
     AND ABS(target_x-1962.64)<0.01 AND ABS(target_y-2336.55)<0.01 AND ABS(target_z-80.3631)<0.01);
COMMIT;
