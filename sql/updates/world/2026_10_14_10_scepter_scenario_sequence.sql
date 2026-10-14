-- An Eye for a Scepter (43153), native scenario 1155 / map 1630.
-- SetData and timed text callbacks have no Unit invoker. Scenario conditions
-- must inspect target 1 (the base actor), not the absent target 0.
UPDATE conditions SET ConditionTarget=1
WHERE SourceTypeOrReferenceId=22 AND SourceId=0 AND SourceEntry IN (109955,109956,110021)
  AND ConditionTypeOrReference=44 AND ConditionValue1=1155;

-- These event objects have only event 0; their link=1 never existed.
UPDATE smart_scripts SET link=0, event_flags=0
WHERE source_type=13 AND entryorguid IN (601,602) AND id=0;

-- Credit arrival independently of optional conversation 2897 (spell 218995).
UPDATE smart_scripts SET action_type=205,action_param1=92,action_param2=52741,
 comment='Scepter - D-Block arrival credit before optional conversation'
WHERE source_type=0 AND entryorguid=109956 AND id=0;
UPDATE smart_scripts SET action_type=11,action_param1=218995,action_param2=2,
 comment='Scepter - arrival conversation (independent of credit)'
WHERE source_type=0 AND entryorguid=109956 AND id=1;
UPDATE smart_scripts SET event_flags=1 WHERE source_type=0 AND entryorguid=109956 AND id IN (2,9,18);
-- Allaris receives SetData without a player invoker. Text callbacks likewise
-- select the nearby player explicitly rather than inheriting stale invokers.
UPDATE smart_scripts SET target_type=21,target_param1=60,target_param2=0
WHERE source_type=0 AND entryorguid=109956 AND id IN (2,10);
UPDATE smart_scripts SET event_flags=1 WHERE source_type=0 AND entryorguid=109955 AND id=5;
UPDATE smart_scripts SET id=18 WHERE source_type=0 AND entryorguid=109955 AND id=6 AND event_param1=3;
UPDATE smart_scripts SET target_type=21,target_param1=60,target_param2=0
WHERE source_type=0 AND entryorguid=109955 AND id=12;

-- Armond's death permits reading the manifest; it does not read it for us.
-- Move to the table first. The actual click starts the reading dialogue.
UPDATE smart_scripts SET link=0,action_type=53,action_param1=1,action_param2=10995601,
 target_type=1,target_param1=0,target_param2=0,
 comment='Scepter - Armond defeated - approach the manifest'
WHERE source_type=0 AND entryorguid=109956 AND id=14;
UPDATE smart_scripts SET event_type=38,event_flags=1,event_param1=7,event_param2=7,
 action_type=1,action_param1=5,action_param2=10000,link=0,
 comment='Scepter - manifest clicked - read then leave for Baradin Hold'
WHERE source_type=0 AND entryorguid=109956 AND id=15;
UPDATE smart_scripts SET link=2,target_param2=60 WHERE source_type=0 AND entryorguid=110323 AND id=1;
UPDATE gameobject_template SET AIName='SmartGameObjectAI',flags=flags|16 WHERE entry=251879;
UPDATE gameobject_template SET AIName='SmartGameObjectAI' WHERE entry=251699;

-- Keep the final enemy alive until Allaris has actually reached the room.
-- The activation is at path point 20, within 40 yards of the boss, rather than
-- point 19 (over 66 yards away, outside the old death callback search).
UPDATE creature_template SET unit_flags=unit_flags|770 WHERE entry IN (109981,110323);
UPDATE smart_scripts SET link=31 WHERE source_type=0 AND entryorguid=109956 AND id=13;
-- Portal creation must not depend on the optional conversation's cast result.
UPDATE smart_scripts SET link=27 WHERE source_type=0 AND entryorguid=109956 AND id=26;
UPDATE smart_scripts SET target_param1=100 WHERE source_type=0 AND entryorguid=109956 AND id IN (25,26);
UPDATE smart_scripts SET action_type=12,action_param1=110800,action_param2=8,
 target_type=8,target_param1=0,target_param2=0,target_x=-129.828,target_y=-4529.59,target_z=170.275,target_o=1.59769,
 comment='Scepter - create return portal before optional conversation'
WHERE source_type=0 AND entryorguid=109956 AND id=27;
UPDATE smart_scripts SET action_type=85,action_param1=219706,action_param2=2,
 target_type=21,target_param1=60,target_param2=0,target_x=0,target_y=0,target_z=0,target_o=0,
 comment='Scepter - optional Eye retrieval conversation'
WHERE source_type=0 AND entryorguid=109956 AND id=28;
UPDATE smart_scripts SET link=0,target_param2=100 WHERE source_type=0 AND entryorguid=109981 AND id=2;
UPDATE smart_scripts SET event_type=38,event_param1=1,event_param2=1,target_type=21,target_param1=60 WHERE source_type=0 AND entryorguid=109964 AND id=2;
UPDATE smart_scripts SET target_type=21,target_param1=60 WHERE source_type=0 AND entryorguid=109964 AND id=4;
DELETE FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceEntry=601 AND SourceId=13 AND SourceGroup=1;
DELETE FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceEntry=602 AND SourceId=13 AND SourceGroup=1;
DELETE FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceEntry=60 AND SourceId=13 AND SourceGroup=1;
DELETE FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceEntry=109956 AND SourceId=0 AND SourceGroup=30;
DELETE FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceEntry=109956 AND SourceId=0 AND SourceGroup=31;
DELETE FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceEntry=109956 AND SourceId=0 AND SourceGroup=16;
DELETE FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceEntry=109956 AND SourceId=0 AND SourceGroup=15;
DELETE FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceEntry=109956 AND SourceId=0 AND SourceGroup=33;
DELETE FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceEntry=109981 AND SourceId=0 AND SourceGroup=4;
DELETE FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceEntry=109981 AND SourceId=0 AND SourceGroup=3;
DELETE FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceEntry=251879 AND SourceId=1 AND SourceGroup=1;
DELETE FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceEntry=251879 AND SourceId=1 AND SourceGroup=3;
DELETE FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceEntry=110030 AND SourceId=0 AND SourceGroup=2;
DELETE FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceEntry=109964 AND SourceId=0 AND SourceGroup=3;
DELETE FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceEntry=109964 AND SourceId=0 AND SourceGroup=10;
-- The entry is shared by the vortex and many unrelated-map effects bunnies.
-- Only this DB spawn is the Baradin Hold entrance.
DELETE FROM smart_scripts WHERE entryorguid=110021 AND source_type=0 AND id IN (1,2,3);
DELETE FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceEntry=110021 AND SourceId=0 AND SourceGroup IN (2,4);
DELETE FROM smart_scripts WHERE entryorguid=-370846 AND source_type=0;
DELETE FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceEntry=-370846 AND SourceId=0;
DELETE FROM smart_scripts WHERE entryorguid=109956 AND source_type=0 AND id IN (29,30,31,32);
DELETE FROM smart_scripts WHERE entryorguid=109964 AND source_type=0 AND id IN (9);
DELETE FROM smart_scripts WHERE entryorguid=109981 AND source_type=0 AND id IN (3,5);
DELETE FROM smart_scripts WHERE entryorguid=110021 AND source_type=0 AND id IN (3);
DELETE FROM smart_scripts WHERE entryorguid=110030 AND source_type=0 AND id IN (1);
DELETE FROM smart_scripts WHERE entryorguid=110323 AND source_type=0 AND id IN (2);
DELETE FROM smart_scripts WHERE entryorguid=251699 AND source_type=1 AND id IN (0);
DELETE FROM smart_scripts WHERE entryorguid=251879 AND source_type=1 AND id IN (0,1,2);
DELETE FROM smart_scripts WHERE entryorguid=60 AND source_type=13 AND id=2;
DELETE FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceEntry=60 AND SourceId=13 AND SourceGroup=3;
INSERT INTO smart_scripts (entryorguid,source_type,id,link,event_type,event_phase_mask,event_chance,event_flags,event_param1,event_param2,event_param3,event_param4,action_type,action_param1,action_param2,action_param3,action_param4,action_param5,action_param6,target_type,target_param1,target_param2,target_param3,comment) VALUES
(60,13,2,0,91,0,100,0,0,0,0,0,45,1,1,0,0,0,0,19,109964,40,0,'Scepter - leave Tyranis to his fate when exiting his room'),
(251699,1,0,0,64,0,100,0,0,0,0,0,204,1155,0,0,0,0,0,7,0,0,0,'Scepter - select correct scenario before native portal teleport'),
(109956,0,29,0,10,0,100,0,1,15,1000,1000,45,1,1,0,0,0,0,1,0,0,0,'Scepter - D-Block arrival recovery near entrance guide'),
(109956,0,30,0,10,0,100,0,1,10,1000,1000,45,3,3,0,0,0,0,1,0,0,0,'Scepter - start entrance dialogue after arrival'),
(109956,0,31,0,61,0,100,0,0,0,0,0,19,770,0,0,0,0,0,19,110323,60,0,'Scepter - allow combat with Armond after guide reaches him'),
(109956,0,32,0,40,0,100,0,20,10995603,0,0,45,1,1,0,0,0,0,19,109981,60,0,'Scepter - guide in final room - activate Occultharon'),
(109981,0,3,5,38,0,100,0,1,1,0,0,19,770,0,0,0,0,0,1,0,0,0,'Scepter - remove encounter protection'),
(109981,0,5,0,61,0,100,0,0,0,0,0,49,0,0,0,0,0,0,21,60,0,0,'Scepter - begin final fight with nearby player'),
(110323,0,2,0,61,0,100,0,0,0,0,0,45,1,1,0,0,0,0,20,251879,30,0,'Scepter - unlock manifest only after Armond dies'),
(251879,1,0,1,38,0,100,0,1,1,0,0,22,1,0,0,0,0,0,1,0,0,0,'Scepter - manifest unlocked'),
(251879,1,1,0,61,0,100,0,0,0,0,0,106,16,0,0,0,0,0,1,0,0,0,'Scepter - make manifest selectable'),
(251879,1,2,0,64,1,100,0,0,0,0,0,45,7,7,0,0,0,0,19,109956,40,0,'Scepter - manifest click tells present guide to read'),
(110030,0,1,0,8,0,100,0,219203,0,0,0,45,1,1,0,0,0,0,19,109964,40,0,'Scepter - chain hook clicked - release Tyranis'),
(109964,0,9,0,8,0,100,0,219203,0,0,0,45,1,1,0,0,0,0,1,0,0,0,'Scepter - prisoner clicked - release Tyranis'),
(-370846,0,1,2,10,0,100,0,1,3,1000,1000,205,92,52900,0,0,0,0,7,0,0,0,'Scepter - enter Baradin Hold'),
(-370846,0,2,0,61,0,100,0,0,0,0,0,85,219338,2,0,0,0,0,7,0,0,0,'Scepter - native Hold interior teleport'),
(-370846,0,3,0,10,0,100,0,1,3,1000,1000,85,219338,2,0,0,0,0,7,0,0,0,'Scepter - retry Hold entrance teleport after failed first cast');
INSERT INTO conditions (SourceTypeOrReferenceId,SourceGroup,SourceEntry,SourceId,ElseGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2,Comment) VALUES
(22,1,601,13,0,44,1,1155,0,'Scepter - base object scenario step 0'),
(22,1,602,13,0,44,1,1155,1,'Scepter - base object scenario step 1'),
(22,1,60,13,0,44,1,1155,2,'Scepter - base object scenario step 2'),
(22,30,109956,0,0,44,1,1155,0,'Scepter - base object scenario step 0'),
(22,31,109956,0,0,44,1,1155,1,'Scepter - base object scenario step 1'),
(22,16,109956,0,0,44,1,1155,7,'Scepter - base object scenario step 7'),
(22,15,109956,0,0,44,1,1155,7,'Scepter - base object scenario step 7'),
(22,33,109956,0,0,44,1,1155,9,'Scepter - base object scenario step 9'),
(22,4,109981,0,0,44,1,1155,9,'Scepter - base object scenario step 9'),
(22,3,109981,0,0,44,1,1155,9,'Scepter - base object scenario step 9'),
(22,1,251879,1,0,44,1,1155,7,'Scepter - base object scenario step 7'),
(22,3,251879,1,0,44,1,1155,7,'Scepter - base object scenario step 7'),
(22,2,110030,0,0,44,1,1155,3,'Scepter - base object scenario step 3'),
(22,3,109964,0,0,44,1,1155,3,'Scepter - base object scenario step 3'),
(22,10,109964,0,0,44,1,1155,3,'Scepter - base object scenario step 3'),
(22,2,-370846,0,0,44,1,1155,8,'Scepter - Hold entrance only at step 8'),
(22,4,-370846,0,0,44,1,1155,9,'Scepter - Hold entrance teleport recovery');
-- The 5-yard trigger is outside Tyranis's alcove. Leaving its north side
-- increases distance to Tyranis beyond 25 yards; walking further into the room
-- does not. Do not resolve the choice simply on the first trigger exit.
INSERT INTO conditions (SourceTypeOrReferenceId,SourceGroup,SourceEntry,SourceId,ElseGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2,NegativeCondition,Comment) VALUES
(22,3,60,13,0,44,1,1155,3,0,'Scepter - undecided Tyranis stage'),
(22,3,60,13,0,29,0,109964,25,1,'Scepter - player leaving the room, not walking towards Tyranis');
-- The second static Allaris is already in the final room. Arrival recovery
-- belongs to the D-Block pair, never that distant actor.
INSERT INTO conditions (SourceTypeOrReferenceId,SourceGroup,SourceEntry,SourceId,ElseGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2,Comment) VALUES
(22,30,109956,0,0,29,1,109955,20,'Scepter - entrance guide near Nagaz'),
(22,31,109956,0,0,29,1,109955,20,'Scepter - entrance guide near Nagaz');
