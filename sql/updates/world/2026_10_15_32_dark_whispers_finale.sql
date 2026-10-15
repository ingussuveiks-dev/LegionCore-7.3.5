-- Poll ritual interruption in combat so an early pull cannot consume its
-- only aggro event before stage 5. Credit players, including a pet's owner.
UPDATE smart_scripts SET link=0 WHERE entryorguid=106652 AND source_type=0 AND id=0 AND action_type=1;
UPDATE smart_scripts SET event_type=0,event_param1=0,event_param2=0,event_param3=1000,event_param4=1000,
    target_type=21,target_param1=100,target_param2=0,comment='Dark Whispers - combat ritual credit to nearby player'
WHERE entryorguid=106652 AND source_type=0 AND id=2 AND action_type=205 AND action_param2=50961;

-- Keep the retreat visual, but its failure must not block the defeat chain.
-- The separate low-health talk starts credit -> skull summon -> despawn.
UPDATE smart_scripts SET link=0,action_type=11,action_param1=152821,action_param2=0,target_type=1,
    comment='Dark Whispers - optional low-health retreat visual'
WHERE entryorguid=106652 AND source_type=0 AND id=3 AND event_type=2;
UPDATE smart_scripts SET event_type=2,event_param1=0,event_param2=8,event_param3=0,event_param4=0,
    action_type=1,action_param1=1,action_param2=0,target_type=21,target_param1=100,target_param2=0,
    comment='Dark Whispers - independent low-health defeat dialogue'
WHERE entryorguid=106652 AND source_type=0 AND id=4 AND link=5;
UPDATE smart_scripts SET target_type=21,target_param1=100,target_param2=0
WHERE entryorguid=106652 AND source_type=0 AND id=5 AND action_type=205 AND action_param2=50962;

-- These polls have no Unit invoker; inspect the base actor's live scenario.
DELETE FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceEntry=106652 AND SourceId=0
    AND SourceGroup IN(3,4,5) AND Comment='Dark Whispers - finale only in stage 5';
INSERT INTO conditions(SourceTypeOrReferenceId,SourceGroup,SourceEntry,SourceId,ElseGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2,Comment) VALUES
(22,3,106652,0,0,44,1,1097,4,'Dark Whispers - finale only in stage 5'),
(22,4,106652,0,0,44,1,1097,4,'Dark Whispers - finale only in stage 5'),
(22,5,106652,0,0,44,1,1097,4,'Dark Whispers - finale only in stage 5');

INSERT INTO instance_template(map,parent,script,allowMount,bonusChance)
SELECT 1498,0,'instance_dark_whispers_artifact',0,0 WHERE NOT EXISTS(SELECT 1 FROM instance_template WHERE map=1498);
UPDATE instance_template SET script='instance_dark_whispers_artifact'
WHERE map=1498 AND script IN('','instance_dark_whispers_artifact');
