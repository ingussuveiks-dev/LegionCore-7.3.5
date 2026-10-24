START TRANSACTION;

SET @xalatath_actor_retry :=
EXISTS (SELECT 1 FROM creature_template WHERE entry=101148 AND AIName='SmartAI' AND ScriptName='npc_xalatath_farthing')
AND EXISTS (SELECT 1 FROM creature_template WHERE entry=102247 AND AIName='SmartAI' AND ScriptName='npc_xalatath_soul_attack')
AND (SELECT COUNT(*) FROM smart_scripts WHERE entryorguid=101148 AND source_type=0)=32
AND (SELECT COUNT(*) FROM smart_scripts WHERE entryorguid=101148 AND source_type=9)=2
AND NOT EXISTS (SELECT 1 FROM smart_scripts WHERE entryorguid=101148
    AND ((event_phase_mask<>0 AND NOT(source_type=0 AND id=30 AND event_phase_mask=1))
      OR (action_type IN(22,23,30,31) AND NOT(source_type=9 AND id=1 AND action_type=22 AND action_param1=1))))
AND NOT EXISTS (SELECT 1 FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceEntry=101148 AND SourceId=0 AND SourceGroup IN(26,27,30,33,34))
AND (SELECT COUNT(*) FROM smart_scripts WHERE entryorguid=101148 AND source_type=0 AND event_phase_mask=0 AND event_chance=100 AND event_flags=1
    AND ((id=25 AND link=26 AND event_type=2 AND event_param1=0 AND event_param2=1 AND action_type=75 AND action_param1=61204 AND target_type=1)
      OR (id=27 AND link=28 AND event_type=61 AND action_type=11 AND action_param1=41062 AND action_param2=0 AND target_type=1)
      OR (id=28 AND link=0 AND event_type=61 AND action_type=11 AND action_param1=201791 AND action_param2=0 AND target_type=1)
      OR (id=29 AND link=0 AND event_type=8 AND event_param1=201899 AND event_param2=0 AND action_type=80 AND action_param1=101148 AND action_param2=2 AND target_type=1)))=4
AND EXISTS (SELECT 1 FROM smart_scripts WHERE entryorguid=101148 AND source_type=0 AND id=26 AND link=27
    AND Difficulties='' AND event_type=61 AND event_phase_mask=0 AND event_chance=100 AND event_flags=1
    AND action_type=12 AND action_param1=102247 AND action_param2=8 AND action_param3=0 AND action_param4=0 AND action_param5=0 AND action_param6=0
    AND target_type=8 AND target_param1=0 AND target_param2=0 AND target_param3=0 AND target_param4=0
    AND ABS(target_x-1868.87)<0.001 AND ABS(target_y-2198.75)<0.001 AND ABS(target_z-38.171)<0.001 AND ABS(target_o-0.141143)<0.001);

INSERT INTO smart_scripts (entryorguid,source_type,id,link,event_type,event_phase_mask,event_chance,event_flags,event_param1,event_param2,event_param3,event_param4,
    action_type,action_param1,action_param2,action_param3,action_param4,action_param5,action_param6,target_type,target_x,target_y,target_z,target_o,comment)
SELECT entryorguid,0,32,0,60,2,100,0,0,0,500,500,action_type,action_param1,action_param2,action_param3,action_param4,action_param5,action_param6,
       target_type,target_x,target_y,target_z,target_o,'Xalatath: retry native soul actor until its actual summon callback'
FROM smart_scripts WHERE entryorguid=101148 AND source_type=0 AND id=26 AND @xalatath_actor_retry;

INSERT INTO smart_scripts (entryorguid,source_type,id,link,event_type,event_phase_mask,event_chance,event_flags,event_param1,action_type,action_param1,target_type,comment)
SELECT 101148,0,33,0,17,2,100,1,102247,22,3,1,'Xalatath: actual owned soul actor stops creation retries' WHERE @xalatath_actor_retry;

UPDATE smart_scripts SET link=0,action_type=22,action_param1=2,action_param2=0,action_param3=0,action_param4=0,action_param5=0,action_param6=0,
    target_type=1,target_x=0,target_y=0,target_z=0,target_o=0,comment='Xalatath: arm native soul actor creation after the low-health transition'
WHERE entryorguid=101148 AND source_type=0 AND id=26 AND @xalatath_actor_retry;

UPDATE smart_scripts SET link=27,comment='Xalatath: real soul attack starts native finale and its existing conversation'
WHERE entryorguid=101148 AND source_type=0 AND id=29 AND @xalatath_actor_retry;

INSERT INTO conditions (SourceTypeOrReferenceId,SourceGroup,SourceEntry,SourceId,ElseGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2,Comment)
SELECT 22,33,101148,0,0,36,1,0,0,'Xalatath: pending soul actor requires living Farthing' WHERE @xalatath_actor_retry
UNION ALL SELECT 22,33,101148,0,0,22,1,1539,0,'Xalatath: pending soul actor remains in the native tomb' WHERE @xalatath_actor_retry
UNION ALL SELECT 22,33,101148,0,0,44,1,991,7,'Xalatath: pending soul actor requires the actual Deacon stage' WHERE @xalatath_actor_retry
UNION ALL SELECT 22,34,101148,0,0,36,1,0,0,'Xalatath: soul actor callback requires living Farthing' WHERE @xalatath_actor_retry
UNION ALL SELECT 22,34,101148,0,0,22,1,1539,0,'Xalatath: soul actor callback remains in the native tomb' WHERE @xalatath_actor_retry
UNION ALL SELECT 22,34,101148,0,0,44,1,991,7,'Xalatath: soul actor callback requires the actual Deacon stage' WHERE @xalatath_actor_retry;

COMMIT;
