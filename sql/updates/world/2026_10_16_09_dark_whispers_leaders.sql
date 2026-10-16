-- Both leaders must die. A ranged player or pet is not a reliable origin for
-- the other-leader proximity check; use the dying actor, not its killer.
UPDATE conditions SET ConditionTarget=1
WHERE SourceTypeOrReferenceId=22 AND SourceId=0 AND ConditionTypeOrReference=29 AND NegativeCondition=1
 AND ((SourceEntry=106825 AND SourceGroup=4 AND ConditionValue1=106827)
   OR (SourceEntry=106827 AND SourceGroup=3 AND ConditionValue1=106825));

-- Native action205 only accepts Player targets; a pet killing blow used to
-- start the linked actions while silently losing the leaders criterion50959.
UPDATE smart_scripts SET target_type=18,target_param1=100,target_param2=0,target_param3=0,
 comment='Dark Whispers - Last leader death - Credit nearby players including pet owners'
WHERE source_type=0 AND ((entryorguid=106825 AND id=3) OR (entryorguid=106827 AND id=2))
 AND action_type=205 AND action_param1=92 AND action_param2=50959;

-- The optional stage5 conversation must be created by the player, not a pet.
-- Force each target to cast on itself, independently of the last invoker.
UPDATE smart_scripts SET action_type=11,action_param2=18,target_type=18,target_param1=100,target_param2=0,target_param3=0,
 comment='Dark Whispers - Last leader death - Player stage5 conversation independent of pet killer'
WHERE source_type=0 AND ((entryorguid=106825 AND id=5) OR (entryorguid=106827 AND id=4))
 AND action_type IN(85,11) AND action_param1=212550;
