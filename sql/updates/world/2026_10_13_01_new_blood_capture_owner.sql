-- The previous fallback lived on Zelifrax: his MovementInform schedules his
-- despawn after 6 seconds, before the 8-second fallback can run. Jagganoth
-- remains for 300 seconds and must own the fallback instead.
DELETE FROM smart_scripts WHERE entryorguid=9879701 AND source_type=9 AND id=13
 AND action_type=45 AND action_param1=1 AND action_param2=1
 AND target_type=19 AND target_param1=98831 AND event_param1=8000;

-- DATA_SET has no invoker. Its condition is evaluated on Jagganoth, not the
-- personal actor that sent the signal. Do not require a surviving, same-phase
-- Ritssyn to find the capture recipient; distinguish the Dalaran ritual from
-- the later Calydus scene by map. Preserve all other conditions.
DELETE FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceGroup=3
 AND SourceEntry=98831 AND SourceId=0 AND ElseGroup=0
 AND ConditionTypeOrReference=29 AND ConditionTarget=0
 AND ConditionValue1=101456 AND ConditionValue2=100 AND NegativeCondition=0;
INSERT IGNORE INTO conditions
 (SourceTypeOrReferenceId,SourceGroup,SourceEntry,SourceId,ElseGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2,ConditionValue3,NegativeCondition,Comment)
VALUES
 (22,3,98831,0,0,22,0,1502,0,0,0,'New Blood capture - Underbelly ritual'),
 (22,3,98831,0,1,22,0,1220,0,0,0,'New Blood capture - Dalaran ritual'),
 (22,5,98831,0,0,22,0,1502,0,0,0,'New Blood fallback - Underbelly only'),
 (22,5,98831,0,1,22,0,1220,0,0,0,'New Blood fallback - Dalaran only');

-- UPDATE (60), not an action inside a timed action list: SetData will start
-- Jagganoth's capture list, and SetScript9 forbids replacing a list while it
-- is being iterated. Both the normal callback and this timer use the SAME
-- non-repeatable DATA_SET event, so they cannot start the capture twice.
INSERT IGNORE INTO smart_scripts
 (entryorguid,source_type,id,event_type,event_chance,event_flags,event_param1,event_param2,
  action_type,action_param1,action_param2,target_type,comment)
VALUES (98831,0,4,60,100,1,18000,18000,45,1,1,1,
 'New Blood - Jagganoth owns capture fallback after Zelifrax challenge');
