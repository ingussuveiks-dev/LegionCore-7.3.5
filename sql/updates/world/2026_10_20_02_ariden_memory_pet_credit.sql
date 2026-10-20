-- Disturbing the Past (warlock/DK/druid): the final memory's killer may be a pet.
-- 198287 is player-only personal kill credit, with SELF as its native target.
-- Have the memory's original summoner self-cast it, independently of the killer.
UPDATE smart_scripts
SET action_type=11, action_param1=198287, action_param2=18,
    target_type=23, target_param1=0, target_param2=0, target_param3=0, target_param4=0,
    comment='Wrathful Memory - Last death - Summoning player self-casts camp credit'
WHERE entryorguid=100735 AND source_type=0 AND id=0 AND event_type=6;

-- Test for the other living memory from the dying actor, not a ranged pet.
UPDATE conditions SET ConditionTarget=1,
    Comment='Wrathful Memory - Last death only - No living memory near dying actor'
WHERE SourceTypeOrReferenceId=22 AND SourceEntry=100735 AND SourceId=0
  AND SourceGroup=1 AND ConditionTypeOrReference=29 AND ConditionValue1=100735
  AND ConditionValue2=30 AND NegativeCondition=1;
