-- The objective is tagging Mosshide with the carried pheromones while disguised.
-- The ensuing dialogue and fight are consequences of the accepted tag.
INSERT IGNORE INTO conditions
    (SourceTypeOrReferenceId,SourceGroup,SourceEntry,SourceId,ElseGroup,ConditionTypeOrReference,
     ConditionTarget,ConditionValue1,ConditionValue2,ConditionValue3,NegativeCondition,Comment)
VALUES
    (17,0,82799,0,0,9,0,26868,0,0,0,'Potent Murloc Pheromone - incomplete Axis of Awful'),
    (17,0,82799,0,0,1,0,82788,0,0,0,'Potent Murloc Pheromone - Clever Plant Disguise'),
    (22,1,44262,0,0,31,0,4,0,0,0,'Mosshide tag - actual player invoker'),
    (22,1,44262,0,0,9,0,26868,0,0,0,'Mosshide tag - incomplete Axis of Awful');

UPDATE smart_scripts
SET action_type=33,action_param1=44266,action_param2=0,target_type=7,target_param1=0,target_param2=0,
    comment='Mosshide - credit the actual pheromone tag'
WHERE entryorguid=44262 AND source_type=0 AND id=0 AND link=1 AND event_type=8 AND event_param1=82799
  AND action_type=45 AND action_param1=1 AND action_param2=1 AND target_type=11 AND target_param1=44263;

UPDATE smart_scripts
SET action_param1=1,target_type=10,target_param1=96528,target_param2=44263,comment='Bluegill leader sniffs the pheromone'
WHERE entryorguid=44262 AND source_type=0 AND id=1 AND link=2 AND event_type=61
  AND action_type=1 AND action_param1=0 AND target_type=1;

DELETE FROM smart_scripts WHERE entryorguid=4426200 AND source_type=9 AND id=1 AND link=0
  AND event_type=0 AND action_type=33 AND action_param1=44266 AND target_type=7;

UPDATE smart_scripts
SET id=10,event_param1=1000,event_param2=1000,comment='Mosshide - shout as the representatives attack'
WHERE entryorguid=4426200 AND source_type=9 AND id=0 AND link=0 AND event_type=0
  AND event_param1=5000 AND event_param2=5000 AND action_type=1 AND action_param1=1;

UPDATE smart_scripts
SET id=11,comment='Mosshide - native end of the scripted attack'
WHERE entryorguid=4426200 AND source_type=9 AND id=2 AND link=0 AND event_type=0 AND action_type=37;

INSERT IGNORE INTO smart_scripts
    (entryorguid,source_type,id,link,event_type,event_chance,event_flags,event_param1,event_param2,
     action_type,action_param1,action_param2,target_type,target_param1,target_param2,comment)
VALUES
    (44263,0,1,0,61,100,0,0,0,1,0,0,1,0,0,'Bluegill leader - roar before the attack'),
    (44263,0,2,0,38,100,1,2,1,80,4426300,2,1,0,0,'Other Bluegill - run the same attack without the leader roar'),
    (4426200,9,0,0,0,100,1,0,0,1,0,0,1,0,0,'Mosshide - notice the pheromone'),
    (4426200,9,1,0,0,100,1,6000,6000,1,2,0,10,96531,44263,'Bluegill - first reply'),
    (4426200,9,2,0,0,100,1,3000,3000,1,2,0,1,0,0,'Mosshide - asks why they are looking'),
    (4426200,9,3,0,0,100,1,3000,3000,1,3,0,10,96530,44263,'Bluegill - second reply'),
    (4426200,9,4,0,0,100,1,4000,4000,45,1,1,10,96528,44263,'Bluegill leader - roar, then native timed attack'),
    (4426200,9,5,0,0,100,1,2000,2000,1,3,0,1,0,0,'Mosshide - final warning'),
    (4426200,9,6,0,0,100,1,0,0,19,512,0,1,0,0,'Mosshide - allow the Bluegill to attack'),
    (4426200,9,7,0,0,100,1,0,0,45,2,1,10,96530,44263,'Second Bluegill joins the attack'),
    (4426200,9,8,0,0,100,1,0,0,45,2,1,10,96531,44263,'Third Bluegill joins the attack'),
    (4426200,9,9,0,0,100,1,3000,3000,1,0,0,10,96525,44264,'Tunnel Rat - cry out as the fight begins');

INSERT IGNORE INTO creature_text
    (CreatureID,GroupID,ID,Text,Type,Language,Probability,BroadcastTextId,Comment)
VALUES
    (44262,2,0,'',12,0,100,44500,'Mosshide - looks at the Bluegill'),
    (44262,3,0,'',12,0,100,44501,'Mosshide - final warning'),
    (44263,1,0,'',12,0,100,44504,'Bluegill leader - sniffs'),
    (44263,2,0,'',12,0,100,44505,'Bluegill - first reply'),
    (44263,3,0,'',12,0,100,44506,'Bluegill - second reply'),
    (44264,0,0,'',14,0,100,44503,'Tunnel Rat - call to flee');
