-- Keep DK/druid's existing reveal handoff. Warlock988 requests exactly one
-- instance-owned stationary guide instead of depending on a transient invoker.
DELETE FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceEntry=101286 AND SourceId=0
 AND SourceGroup=14 AND Comment LIKE 'Dark Riders reveal handoff:%';
INSERT INTO conditions(SourceTypeOrReferenceId,SourceGroup,SourceEntry,SourceId,ElseGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2,Comment) VALUES
(22,14,101286,0,0,44,1,1026,3,'Dark Riders reveal handoff: native DK'),
(22,14,101286,0,1,44,1,1014,3,'Dark Riders reveal handoff: native druid');
DELETE FROM smart_scripts WHERE entryorguid=101286 AND source_type=0 AND id=92 AND comment='Dark Riders reveal - exclusive Revil handoff';
INSERT INTO smart_scripts(entryorguid,source_type,id,link,event_type,event_chance,event_flags,event_param1,event_param2,action_type,action_param1,action_param2,target_type,comment) VALUES
(101286,0,92,0,52,100,1,1,101286,34,10085003,1,1,'Dark Riders reveal - exclusive Revil handoff');
DELETE FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceEntry=101286 AND SourceId=0
 AND SourceGroup=93 AND Comment LIKE 'Dark Riders reveal handoff:%';
INSERT INTO conditions(SourceTypeOrReferenceId,SourceGroup,SourceEntry,SourceId,ElseGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2,Comment) VALUES
(22,93,101286,0,0,44,1,988,3,'Dark Riders reveal handoff: Warlock instance request');
