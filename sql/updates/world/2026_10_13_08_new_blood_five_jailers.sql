-- Native 200038 forcecasts 200032 on the five surviving warlocks. A quest-only
-- caster condition blocked the four NPCs. Keep pets, Zelifrax and bystanders out;
-- the core's quest-scope target filter also isolates simultaneous players.
DELETE FROM conditions WHERE (SourceTypeOrReferenceId=13 AND SourceEntry=200038 AND SourceGroup=1) OR (SourceTypeOrReferenceId=17 AND SourceEntry=200032 AND SourceGroup=0);
INSERT INTO conditions (SourceTypeOrReferenceId,SourceGroup,SourceEntry,ElseGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2,Comment) VALUES
(13,1,200038,0,31,0,4,0,'New Blood - participating player'),
(13,1,200038,0,9,0,40729,0,'New Blood - active quest'),
(13,1,200038,1,31,0,3,101456,'New Blood - Ritssyn'),
(13,1,200038,2,31,0,3,101454,'New Blood - council'),
(13,1,200038,3,31,0,3,101455,'New Blood - council'),
(13,1,200038,4,31,0,3,101457,'New Blood - council'),
(17,0,200032,0,9,0,40729,0,'New Blood - player summons captor'),
(17,0,200032,1,31,0,3,101456,'New Blood - Ritssyn summons captor'),
(17,0,200032,2,31,0,3,101454,'New Blood - council summons captor'),
(17,0,200032,3,31,0,3,101455,'New Blood - council summons captor'),
(17,0,200032,4,31,0,3,101457,'New Blood - council summons captor');

-- Face the individual summoner, then channel native Imprisoned (visual 51979).
-- 196229 remains the arrival effect; it is not the sustained capture channel.
UPDATE smart_scripts SET action_type=66,action_param1=0,action_param2=0,target_type=23,link=5,comment='New Blood - jailer faces own captive' WHERE entryorguid=101460 AND source_type=0 AND id=1;
DELETE FROM smart_scripts WHERE entryorguid=101460 AND source_type=0 AND id=5;
INSERT INTO smart_scripts (entryorguid,source_type,id,link,event_type,event_chance,event_flags,action_type,action_param1,action_param2,target_type,comment) VALUES
(101460,0,5,0,61,100,1,11,200033,0,23,'New Blood - channel native imprisonment on own captive');
-- Only the player's captor may advance/teleport the player. NPC captors channel
-- until the same attempt's existing cleanup removes all five.
-- Use an independent event: linked actions bypass source-22 conditions.
UPDATE smart_scripts SET event_type=54,link=0 WHERE entryorguid=101460 AND source_type=0 AND id=2;
DELETE FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceEntry=101460 AND SourceGroup=3;
INSERT INTO conditions (SourceTypeOrReferenceId,SourceGroup,SourceEntry,ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2,Comment) VALUES
(22,3,101460,31,0,4,0,'New Blood - only player captor runs transfer'),
(22,3,101460,9,0,40729,0,'New Blood - active capture');

-- Preserve the existing 12-second capture interval, then allow the native
-- one-second blackout to play before teleporting. The former linked cast
-- teleported immediately when the blackout began.
DELETE FROM spell_linked_spell WHERE spell_trigger=199362 AND spell_effect=199359;
DELETE FROM smart_scripts WHERE entryorguid=101460 AND source_type=9;
INSERT INTO smart_scripts (entryorguid,source_type,id,link,event_type,event_chance,event_flags,event_param1,event_param2,action_type,action_param1,action_param2,target_type,target_param1,target_param2,comment) VALUES
(101460,9,0,4,0,100,1,12000,12000,85,199362,2,7,0,0,'New Blood - blackout after twelve seconds of imprisonment'),
(101460,9,1,0,0,100,1,1000,1000,85,199359,2,7,0,0,'New Blood - native prison transfer after blackout'),
(101460,9,2,0,0,100,1,0,0,45,2,2,19,101456,50,'New Blood - remove Ritssyn'),
(101460,9,3,0,0,100,1,0,0,45,3,3,19,101457,50,'New Blood - remove council'),
(101460,9,4,0,0,100,1,0,0,45,3,3,19,101455,50,'New Blood - remove council'),
(101460,9,5,0,0,100,1,0,0,45,3,3,19,101454,50,'New Blood - remove council'),
(101460,9,6,0,0,100,1,0,0,45,2,2,11,35845,50,'New Blood - remove ritual helpers'),
(101460,9,7,0,0,100,1,0,0,45,2,2,19,98831,50,'New Blood - remove Jagganoth'),
(101460,9,9,0,0,100,1,0,0,45,1,1,11,101460,50,'New Blood - remove this attempt captors');
