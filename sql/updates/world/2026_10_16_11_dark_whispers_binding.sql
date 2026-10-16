-- Last doomweaver death already summons Mephistroth. Remove the binding in
-- that same death chain, before his first update/approach, not at skull pickup.
UPDATE smart_scripts SET link=5
WHERE entryorguid=106698 AND source_type=0 AND id=3 AND action_type=12 AND action_param1=106692;
DELETE FROM smart_scripts WHERE entryorguid=106698 AND source_type=0 AND id=5;
INSERT INTO smart_scripts (entryorguid,source_type,id,event_type,event_chance,action_type,action_param1,target_type,target_param1,target_param2,comment)
 VALUES (106698,0,5,61,100,28,211683,19,106693,50,'Dark Whispers - Last doomweaver death - Remove skull binding before approach');

-- The living-doomweaver check belongs to the dying actor, not a ranged pet.
UPDATE conditions SET ConditionTarget=1
WHERE SourceTypeOrReferenceId=22 AND SourceEntry=106698 AND SourceId=0 AND SourceGroup=4
 AND ConditionTypeOrReference=29 AND ConditionValue1=106698 AND NegativeCondition=1;
