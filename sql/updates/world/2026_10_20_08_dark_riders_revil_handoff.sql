-- Scenario988 uses Player arrival and one instance-owned guide request.
-- Stop the old follower's Data2/2 route and its late point1 summon callback
-- from creating another stationary guide after the same transition.
DELETE FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceEntry=101286 AND SourceId=0
 AND SourceGroup=4 AND ConditionTypeOrReference=44 AND ConditionValue1=988;
DELETE FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceEntry=101286 AND SourceId=0
 AND SourceGroup=5 AND Comment LIKE 'Dark Riders Revil handoff:%';
INSERT INTO conditions(SourceTypeOrReferenceId,SourceGroup,SourceEntry,SourceId,ElseGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2,ConditionValue3,NegativeCondition,ErrorTextId,ScriptName,Comment) VALUES
(22,5,101286,0,0,44,1,1026,1,0,0,0,'','Dark Riders Revil handoff: native DK point1 route'),
(22,5,101286,0,1,44,1,1014,1,0,0,0,'','Dark Riders Revil handoff: native druid point1 route');
UPDATE smart_scripts SET action_type=34,action_param1=10085001,action_param2=1,
 action_param3=0,action_param4=0,action_param5=0,action_param6=0,
 target_type=1,target_param1=0,target_param2=0,target_param3=0,target_param4=0,
 target_x=0,target_y=0,target_z=0,target_o=0,
 comment='Dark Riders barrier - idempotent instance-owned Revil handoff'
WHERE entryorguid=-369334 AND source_type=0 AND id=8 AND event_type=61;
