-- The archive summon is owned by Calydus. Its scenario-local AI waits for the
-- final step and runs the confrontation once, retaining that exact guide GUID.
UPDATE creature_template SET ScriptName='npc_new_blood_jagganoth' WHERE entry=98831;
DELETE FROM smart_scripts WHERE entryorguid=98831 AND source_type=0 AND id=5;
DELETE FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceEntry=98831 AND SourceGroup=6 AND SourceId=0;
DELETE FROM smart_scripts WHERE entryorguid=9883101 AND source_type=9;

-- Restart Birth explicitly after the create packet, then allow all 13 seconds
-- before enabling spellclick. Keep the pre-create Birth state in Map.cpp.
DELETE FROM smart_scripts WHERE entryorguid=101433 AND source_type=0 AND id=9;
INSERT INTO smart_scripts
 (entryorguid,source_type,id,event_type,event_chance,event_flags,event_param1,event_param2,action_type,action_param1,action_param2,target_type,comment) VALUES
 (101433,0,9,60,100,1,200,200,128,1254,0,1,'New Blood - explicit full Birth playback after portal creation');
UPDATE smart_scripts SET event_param1=13200,event_param2=13200
 WHERE entryorguid=101433 AND source_type=0 AND id=7;

-- A failed conversation cast returns before its LINK action. Schedule flames
-- as their own timed action; never make them depend on a successful dialogue.
UPDATE smart_scripts SET link=0 WHERE entryorguid=9879701 AND source_type=9 AND id=9;
-- Reserve id 10 without moving rows again when the migration is replayed.
UPDATE smart_scripts SET id=13 WHERE entryorguid=9879701 AND source_type=9 AND id=12 AND action_type=69;
UPDATE smart_scripts SET id=12 WHERE entryorguid=9879701 AND source_type=9 AND id=11 AND action_type=11 AND action_param1=199945;
UPDATE smart_scripts SET id=11 WHERE entryorguid=9879701 AND source_type=9 AND id=10 AND action_type=12 AND action_param1=98831;
DELETE FROM smart_scripts WHERE entryorguid=9879701 AND source_type=9 AND id=10;
INSERT INTO smart_scripts
 (entryorguid,source_type,id,event_type,event_chance,event_flags,action_type,action_param1,action_param2,target_type,target_param1,target_param2,comment) VALUES
 (9879701,9,10,0,100,1,128,1244,0,19,101433,40,'New Blood - independent native portal flame animation');

-- SpellVisual 51979's cast kit 4911 carries the native hand attachments.
-- Keep it on each jailer for the capture interval, independent of a failed
-- captive-target cast or its caster-side aura filter.
DELETE FROM smart_scripts WHERE entryorguid=101460 AND source_type=0 AND id=10;
INSERT INTO smart_scripts
 (entryorguid,source_type,id,event_type,event_chance,event_flags,event_param1,event_param2,action_type,action_param1,action_param2,action_param3,target_type,comment) VALUES
 (101460,0,10,60,100,1,250,250,27,4911,0,12000,1,'New Blood - native jailer hand visual on the jailer only');
