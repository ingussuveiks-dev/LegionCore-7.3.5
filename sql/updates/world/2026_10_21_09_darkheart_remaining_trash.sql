-- Native 198792 is Prancing Escape: a collision-limited backward destination
-- charge. Face the actual tank (including a pet) before choosing "back".
DELETE FROM smart_scripts WHERE entryorguid=99358 AND source_type=0 AND id IN(2,3);
INSERT INTO smart_scripts (entryorguid,source_type,id,link,event_type,event_param1,event_param2,event_param3,event_param4,action_type,action_param1,target_type,comment) VALUES
 (99358,0,2,3,9,0,8,12000,15000,66,0,2,'Rotheart Dryad - face close victim before Prancing Escape'),
 (99358,0,3,0,61,0,0,0,0,11,198792,1,'Rotheart Dryad - native backward Prancing Escape');

-- Inclusive 0..threshold windows cannot be skipped by a nonlethal burst hit.
-- Retain their one-shot flags and all other timers/actions.
UPDATE smart_scripts SET event_param1=0,event_param2=65,comment='Festerhide Grizzly - Maddening Roar at or below 65 percent'
 WHERE entryorguid=95779 AND source_type=0 AND event_type=2 AND action_type=11 AND action_param1=200580;
UPDATE smart_scripts SET event_param1=0,event_param2=95
 WHERE entryorguid=95779 AND source_type=0 AND event_type=2 AND action_type=11 AND action_param1=200589;
UPDATE smart_scripts SET event_param1=0,event_param2=50,target_type=125,comment='Dreadfire Imp - Dread Inferno at or below 50 percent at a player'
 WHERE entryorguid=100527 AND source_type=0 AND event_type=2 AND action_type=11 AND action_param1=201399;

UPDATE creature_template SET AIName='',ScriptName='npc_darkheart_bloodtainted_burster' WHERE entry=100532;
-- Native Metamorphosis creates the separate Fury template. It gets the two
-- original combat abilities, but deliberately no Dispersion death action.
UPDATE creature_template SET AIName='SmartAI',ScriptName='' WHERE entry=113398;
DELETE FROM smart_scripts WHERE entryorguid=113398 AND source_type=0 AND id IN(0,1);
INSERT INTO smart_scripts (entryorguid,source_type,id,event_type,event_param1,event_param2,event_param3,event_param4,action_type,action_param1,target_type,comment) VALUES
 (113398,0,0,0,7000,8000,14000,16000,11,201226,5,'Regenerated Bloodtainted Fury - Blood Assault'),
 (113398,0,1,0,15000,16000,30000,32000,11,201272,5,'Regenerated Bloodtainted Fury - Blood Bomb');
