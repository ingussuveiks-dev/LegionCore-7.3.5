-- SetData has no unit invoker. SmartAI's first condition check supplies
-- (nullptr, baseObject), unlike its later ProcessEvent fallback. Check the
-- base object's map, otherwise both the movement and timer signals are lost.
UPDATE conditions SET ConditionTarget=1
WHERE SourceTypeOrReferenceId=22 AND SourceEntry=98831 AND SourceId=0
 AND SourceGroup IN(3,5) AND ConditionTypeOrReference=22
 AND ConditionValue1 IN(1220,1502) AND ConditionTarget=0;

-- The participant is now actually stunned: the jailer's blackout must be
-- triggered, then release the intro-only stun for the prison scenario.
UPDATE smart_scripts SET action_param2=2,link=4
WHERE entryorguid=101460 AND source_type=9 AND id=0 AND action_type=85
 AND action_param1=199362 AND link=0;
INSERT IGNORE INTO smart_scripts
 (entryorguid,source_type,id,event_type,event_chance,event_flags,action_type,action_param1,target_type,comment)
VALUES (101460,0,4,61,100,1,28,204833,7,'New Blood - clear intro stun after jailer blackout');

-- Model 607277 (display 68219) has a 13000ms ANIM_BIRTH. The core initializes
-- that animation before visibility; stop it and permit interaction at its end.
-- Ground height is the previous static map-1502 portal's 696.66, not 708.5.
UPDATE smart_scripts SET target_z=696.66
WHERE entryorguid=98797 AND source_type=0 AND id=2 AND action_type=12
 AND action_param1=101433 AND target_z=708.5;

-- The glowing interior is shown while channeling, not while building the frame.
INSERT IGNORE INTO smart_scripts
 (entryorguid,source_type,id,link,event_type,event_chance,event_flags,
 action_type,action_param1,action_param2,target_type,target_x,target_y,target_z,target_o,comment)
SELECT 101433,0,6,0,61,100,1,50,245958,0,8,target_x,target_y,target_z,target_o,
 'New Blood - open portal interior on ritual click'
FROM smart_scripts WHERE entryorguid=98797 AND source_type=0 AND id=3
 AND action_type=50 AND action_param1=245958;
UPDATE smart_scripts SET link=4 WHERE entryorguid=98797 AND source_type=0
 AND id=2 AND action_type=12 AND action_param1=101433 AND link=3;
DELETE FROM smart_scripts WHERE entryorguid=98797 AND source_type=0 AND id=3
 AND action_type=50 AND action_param1=245958;
UPDATE smart_scripts SET link=6 WHERE entryorguid=101433 AND source_type=0 AND id=5 AND action_type=81 AND action_param1=0;
INSERT IGNORE INTO smart_scripts
 (entryorguid,source_type,id,link,event_type,event_chance,event_flags,event_param1,event_param2,
 action_type,action_param1,action_param2,target_type,comment)
VALUES
 (101433,0,7,8,60,100,1,13000,13000,128,0,1,1,'New Blood - finish native portal Birth animation'),
 (101433,0,8,0,61,100,1,0,0,81,16777216,0,1,'New Blood - portal ready after assembly');
UPDATE smart_scripts SET event_param1=8000,event_param2=8000
WHERE entryorguid=9879700 AND source_type=9 AND id=1 AND action_type=11
 AND action_param1=199930 AND event_param1=5000 AND event_param2=5000;

-- Reignite the completed frame at the warning before Jagganoth arrives.
-- Aura 199994 is the native Portal Malfunction visual (SpellVisual 51954).
UPDATE smart_scripts SET link=13 WHERE entryorguid=9879701 AND source_type=9
 AND id=9 AND action_type=11 AND action_param1=199941 AND link=0;
-- Links resolve against the actor's ordinary event list, not the timed list.
INSERT IGNORE INTO smart_scripts
 (entryorguid,source_type,id,event_type,event_chance,event_flags,action_type,action_param1,
 target_type,target_param1,target_param2,comment)
VALUES (98797,0,13,61,100,1,75,199994,19,101433,40,'New Blood - reignite personal portal before Jagganoth');
