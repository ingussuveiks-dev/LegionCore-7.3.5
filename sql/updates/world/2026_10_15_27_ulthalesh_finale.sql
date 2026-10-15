-- Keep the native boss dialogue, criteria and personal artifact/guide actors.
-- The earlier To Point the Way compass has real quest loot but no template link.
UPDATE gameobject_template SET Data1=245794
WHERE entry=245794 AND type=3 AND Data1 IN(0,245794) AND ScriptName='';

-- An optional cast must not consume the required low-health transition.
UPDATE smart_scripts SET link=0 WHERE entryorguid=100344 AND source_type=0 AND id=10 AND action_type=11 AND action_param1=199764;
DELETE FROM smart_scripts WHERE entryorguid=100344 AND source_type=0 AND id=90 AND comment='Ulthalesh - independent low-health finale';
INSERT INTO smart_scripts(entryorguid,source_type,id,link,Difficulties,event_type,event_phase_mask,event_chance,event_flags,event_param1,event_param2,event_param3,event_param4,event_param5,action_type,action_param1,action_param2,action_param3,action_param4,action_param5,action_param6,target_type,target_param1,target_param2,target_param3,target_param4,target_x,target_y,target_z,target_o,comment)
SELECT entryorguid,source_type,90,12,Difficulties,2,event_phase_mask,event_chance,event_flags,0,1,0,0,0,action_type,action_param1,action_param2,action_param3,action_param4,action_param5,action_param6,target_type,target_param1,target_param2,target_param3,target_param4,target_x,target_y,target_z,target_o,'Ulthalesh - independent low-health finale'
FROM smart_scripts WHERE entryorguid=100344 AND source_type=0 AND id=11 AND action_type=1 AND action_param1=3;

-- Do not expose the chest during the five-second defeat speech: its pickup
-- criterion belongs to the next step, after the actual defeat criterion.
UPDATE smart_scripts SET link=0 WHERE entryorguid=100344 AND source_type=0 AND id=12 AND action_type=21;
UPDATE smart_scripts SET link=15 WHERE entryorguid=100344 AND source_type=0 AND id=14 AND action_type=205 AND action_param2=47989;
UPDATE smart_scripts SET link=16 WHERE entryorguid=100344 AND source_type=0 AND id=15 AND action_type=50 AND action_param1=246021;
UPDATE smart_scripts SET event_type=37 WHERE entryorguid=100344 AND source_type=0 AND id=9 AND action_type=42 AND action_param1=1 AND event_type IN(37,60);

-- The finale guard uses the existing post-scaling instance damage hook, scoped
-- to scenario988/Ariden100344. Other scenarios on map1533 retain their actors.
INSERT INTO instance_template(map,parent,script,allowMount,bonusChance)
SELECT 1533,0,'instance_dark_riders_artifact',0,0 WHERE NOT EXISTS(SELECT 1 FROM instance_template WHERE map=1533);
UPDATE instance_template SET script='instance_dark_riders_artifact' WHERE map=1533 AND script IN('','instance_dark_riders_artifact');
