START TRANSACTION;

SET @xalatath_weapon_retry :=
EXISTS (SELECT 1 FROM creature_template WHERE entry=101148 AND AIName='SmartAI' AND ScriptName='npc_xalatath_farthing')
AND EXISTS (SELECT 1 FROM gameobject_template WHERE entry=247690 AND type=3 AND Data0=1691 AND Data1=247690 AND AIName='SmartGameObjectAI' AND ScriptName='go_xalatath_pickup')
AND (SELECT COUNT(*) FROM smart_scripts WHERE entryorguid=101148 AND source_type=0)=30
AND (SELECT COUNT(*) FROM smart_scripts WHERE entryorguid=101148 AND source_type=9)=3
AND NOT EXISTS (SELECT 1 FROM smart_scripts WHERE entryorguid=101148 AND (event_phase_mask<>0 OR action_type IN(22,23,30,31)))
AND NOT EXISTS (SELECT 1 FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceEntry=101148 AND SourceGroup IN(31,32))
AND EXISTS (SELECT 1 FROM smart_scripts WHERE entryorguid=101148 AND source_type=0 AND id=29 AND link=0
    AND event_type=8 AND event_flags=1 AND event_param1=201899 AND action_type=80 AND action_param1=101148 AND action_param2=2 AND target_type=1)
AND EXISTS (SELECT 1 FROM smart_scripts WHERE entryorguid=101148 AND source_type=9 AND id=0 AND link=0
    AND event_type=0 AND event_flags=1 AND event_param1=20000 AND event_param2=20000
    AND action_type=205 AND action_param1=92 AND action_param2=48421 AND target_type=21 AND target_param1=50)
AND EXISTS (SELECT 1 FROM smart_scripts WHERE entryorguid=101148 AND source_type=9 AND id=1 AND link=0
    AND Difficulties='' AND event_type=0 AND event_chance=100 AND event_flags=1
    AND event_param1=0 AND event_param2=0 AND event_param3=0 AND event_param4=0 AND event_param5=0
    AND action_type=50 AND action_param1=247690 AND action_param2=3600 AND action_param3=0 AND action_param4=0 AND action_param5=0 AND action_param6=0
    AND target_type=8 AND target_param1=0 AND target_param2=0 AND target_param3=0 AND target_param4=0
    AND ABS(target_x-1868.87)<0.001 AND ABS(target_y-2198.75)<0.001 AND ABS(target_z-37.3797)<0.001 AND ABS(target_o-0.141143)<0.001)
AND EXISTS (SELECT 1 FROM smart_scripts WHERE entryorguid=101148 AND source_type=9 AND id=2 AND link=0
    AND event_type=0 AND event_flags=1 AND event_param1=0 AND event_param2=0 AND action_type=37 AND target_type=1);

INSERT INTO smart_scripts (entryorguid,source_type,id,link,event_type,event_phase_mask,event_chance,event_flags,event_param1,event_param2,event_param3,event_param4,
    action_type,action_param1,action_param2,action_param3,target_type,target_x,target_y,target_z,target_o,comment)
SELECT entryorguid,0,30,31,60,1,100,1,0,0,500,500,50,action_param1,action_param2,2,target_type,target_x,target_y,target_z,target_o,
       'Xalatath: retain pending weapon creation until the actual native summon succeeds'
FROM smart_scripts WHERE entryorguid=101148 AND source_type=9 AND id=1 AND @xalatath_weapon_retry;

INSERT INTO smart_scripts (entryorguid,source_type,id,link,event_type,event_phase_mask,event_chance,event_flags,action_type,target_type,comment)
SELECT entryorguid,0,31,0,61,0,100,1,action_type,target_type,'Xalatath: native finale death only after successful weapon creation'
FROM smart_scripts WHERE entryorguid=101148 AND source_type=9 AND id=2 AND @xalatath_weapon_retry;

UPDATE smart_scripts SET action_type=22,action_param1=1,action_param2=0,target_type=1,target_x=0,target_y=0,target_z=0,target_o=0,
    comment='Xalatath: arm pending weapon creation after the native twenty-second finale'
WHERE entryorguid=101148 AND source_type=9 AND id=1 AND @xalatath_weapon_retry;

DELETE FROM smart_scripts WHERE entryorguid=101148 AND source_type=9 AND id=2 AND @xalatath_weapon_retry;

INSERT INTO conditions (SourceTypeOrReferenceId,SourceGroup,SourceEntry,SourceId,ElseGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2,Comment)
SELECT 22,31,101148,0,0,36,1,0,0,'Xalatath: pending weapon creation requires living Farthing' WHERE @xalatath_weapon_retry
UNION ALL SELECT 22,31,101148,0,0,22,1,1539,0,'Xalatath: pending weapon creation remains in the native tomb' WHERE @xalatath_weapon_retry
UNION ALL SELECT 22,31,101148,0,0,44,1,991,8,'Xalatath: pending weapon creation follows actual Deacon-stage credit' WHERE @xalatath_weapon_retry;

COMMIT;
