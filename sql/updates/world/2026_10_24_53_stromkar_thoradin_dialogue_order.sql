START TRANSACTION;

SET @stromkar_dialogue := EXISTS (SELECT 1 FROM creature_template WHERE entry=104307 AND AIName='SmartAI' AND ScriptName='' AND npcflag=0)
AND EXISTS (SELECT 1 FROM creature_template WHERE entry=104276 AND AIName='SmartAI' AND ScriptName='npc_stromkar_zakajz')
AND (SELECT COUNT(*) FROM smart_scripts WHERE entryorguid=104307)=5
AND EXISTS (SELECT 1 FROM smart_scripts WHERE entryorguid=104307 AND source_type=0 AND id=0 AND link=2 AND event_type=54 AND event_phase_mask=0 AND event_chance=100 AND event_flags=1
    AND action_type=1 AND action_param1=0 AND action_param2=6000 AND target_type=1)
AND EXISTS (SELECT 1 FROM smart_scripts WHERE entryorguid=104307 AND source_type=0 AND id=1 AND link=0 AND event_type=52 AND event_phase_mask=0 AND event_chance=100 AND event_flags=1
    AND event_param1=0 AND event_param2=104307 AND action_type=1 AND action_param1=1 AND action_param2=8000 AND target_type=1)
AND EXISTS (SELECT 1 FROM smart_scripts WHERE entryorguid=104307 AND source_type=0 AND id=2 AND link=0 AND event_type=61 AND event_phase_mask=0 AND event_chance=100 AND event_flags=1
    AND action_type=53 AND action_param2=104307 AND target_type=1)
AND EXISTS (SELECT 1 FROM smart_scripts WHERE entryorguid=104307 AND source_type=0 AND id=3 AND link=0 AND event_type=38 AND event_phase_mask=0 AND event_chance=100 AND event_flags=1
    AND event_param1=1 AND event_param2=1 AND action_type=1 AND action_param1=2 AND action_param2=8000 AND target_type=1)
AND EXISTS (SELECT 1 FROM smart_scripts WHERE entryorguid=104307 AND source_type=0 AND id=4 AND link=0 AND event_type=52 AND event_phase_mask=0 AND event_chance=100 AND event_flags=1
    AND event_param1=2 AND event_param2=104307 AND action_type=1 AND action_param1=3 AND action_param2=8000 AND target_type=1)
AND NOT EXISTS (SELECT 1 FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceEntry=104307);

UPDATE creature_template SET ScriptName='npc_stromkar_thoradin' WHERE entry=104307 AND @stromkar_dialogue;

INSERT INTO smart_scripts (entryorguid,source_type,id,link,event_type,event_phase_mask,event_chance,event_flags,event_param1,event_param2,event_param3,event_param4,
    action_type,action_param1,action_param2,action_param3,action_param4,action_param5,action_param6,target_type,target_param1,target_param2,target_param3,target_x,target_y,target_z,target_o,comment)
SELECT 104307,0,5,0,52,0,100,1,1,104307,0,0,45,2,1,0,0,0,0,1,0,0,0,0,0,0,0,
    'Stromkar: release queued finale only after native Thoradin take-up-the-blade text ends'
WHERE @stromkar_dialogue;

COMMIT;
