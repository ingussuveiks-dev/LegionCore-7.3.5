-- The first transition belongs to the Player reaching the barrier, not to
-- Revil's transient follower entering Ariden's radius. Scope the replacement
-- to the Warlock-phase static actor; shared/reveal Aridens keep their scripts.
-- Restore the stair-local introduction; the former20-yard recovery started
-- the speech/companion immediately at the recorded entrance instead.
UPDATE smart_scripts SET event_param2=2 WHERE entryorguid=-369378 AND source_type=0 AND id=0 AND event_type=10;
DELETE FROM smart_scripts WHERE entryorguid=-369334 AND source_type=0;
INSERT INTO smart_scripts
SELECT -369334,source_type,id,link,Difficulties,event_type,event_phase_mask,event_chance,event_flags,
 event_param1,event_param2,event_param3,event_param4,event_param5,
 action_type,action_param1,action_param2,action_param3,action_param4,action_param5,action_param6,
 target_type,target_param1,target_param2,target_param3,target_param4,target_x,target_y,target_z,target_o,
 CONCAT('Dark Riders barrier - ',comment)
FROM smart_scripts WHERE entryorguid=100850 AND source_type=0;
DELETE FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceEntry=-369334 AND SourceId=0;
INSERT INTO conditions
SELECT SourceTypeOrReferenceId,SourceGroup,-369334,SourceId,ElseGroup,ConditionTypeOrReference,
 ConditionTarget,ConditionValue1,ConditionValue2,ConditionValue3,NegativeCondition,ErrorTextId,ScriptName,Comment
FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceEntry=100850 AND SourceId=0 AND SourceGroup<>8;
INSERT INTO conditions(SourceTypeOrReferenceId,SourceGroup,SourceEntry,SourceId,ElseGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2,ConditionValue3,NegativeCondition,ErrorTextId,ScriptName,Comment) VALUES
(22,8,-369334,0,0,44,1,988,0,0,0,0,'','Dark Riders: barrier arrival only in first step'),
(22,8,-369334,0,0,32,0,16,0,0,0,0,'','Dark Riders: Player arrival, never a pet or follower');
UPDATE smart_scripts SET link=8,event_type=10,event_flags=1,event_param1=0,event_param2=48,
 event_param3=0,event_param4=0,event_param5=0,action_type=205,
 action_param1=92,action_param2=47983,action_param3=0,action_param4=0,action_param5=0,action_param6=0,
 target_type=7,target_param1=0,target_param2=0,target_param3=0,target_param4=0,
 comment='Dark Riders barrier - Player arrival credits infiltration independently of follower'
WHERE entryorguid=-369334 AND source_type=0 AND id=7;
-- Credit first: the stationary guide's JUST_SUMMONED conversation requires
-- step1. Reuse the native guide position, scene and 16-second dialogue timer.
INSERT INTO smart_scripts(entryorguid,source_type,id,link,event_type,event_chance,event_flags,action_type,action_param1,action_param2,target_type,target_x,target_y,target_z,target_o,comment) VALUES
(-369334,0,8,9,61,100,1,12,101282,8,8,-10962.5,-1970.73,-49.8162,1.513,'Dark Riders barrier - create native stationary Revil after stage transition'),
(-369334,0,9,0,61,100,1,85,199627,2,7,0,0,0,0,'Dark Riders barrier - Player unsummons follower even when it was left on stairs');
