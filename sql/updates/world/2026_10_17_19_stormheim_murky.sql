-- A Murky Fate (40120): one completed torch hit consumes one corpse lifecycle.
-- Keep the native 5-yard area spell and 10-second Burning Body visual.
UPDATE smart_scripts SET event_flags=event_flags|1,link=1
WHERE entryorguid=98577 AND source_type=0 AND id=0
  AND event_type=8 AND event_param1=194067 AND action_type=33 AND action_param1=98577;
UPDATE smart_scripts SET event_type=61,event_flags=0,event_param1=0,event_param2=0,
    event_param3=0,event_param4=0,event_param5=0,link=2
WHERE entryorguid=98577 AND source_type=0 AND id=1 AND action_type=75 AND action_param1=194069;
INSERT IGNORE INTO smart_scripts
(entryorguid,source_type,id,link,event_type,event_chance,action_type,action_param1,target_type,comment)
VALUES (98577,0,2,0,61,100,41,10000,1,'Decaying Corpse - After native burning visual - Despawn and use normal spawn respawn');

-- Source22 group is event ID+1. Gate before the one-shot is consumed; LINK
-- visual/cleanup must still run when the eighth credit completes the quest.
INSERT IGNORE INTO conditions
(SourceTypeOrReferenceId,SourceGroup,SourceEntry,SourceId,ElseGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2,ConditionValue3,NegativeCondition,Comment)
VALUES
(22,1,98577,0,0,31,0,4,0,0,0,'Murky Fate - torch invoker must be a player'),
(22,1,98577,0,0,9,0,40120,0,0,0,'Murky Fate - player has incomplete quest before credit'),
(22,1,98577,0,0,1,1,194069,0,0,1,'Murky Fate - base corpse is not already burning'),
(13,1,194067,0,0,31,0,3,98577,0,0,'Burning Body - only Decaying Corpse area targets'),
(13,1,194067,0,0,1,0,194069,0,0,1,'Burning Body - exclude already burning corpses');
