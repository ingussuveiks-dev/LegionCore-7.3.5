-- Creature/GO/timed-action scenario-step conditions concern the base actor's
-- instance. SetData/text/waypoint callbacks may have no Unit invoker at the
-- initial IsObjectMeetingSmartEventConditions gate (before ProcessEvent's fallback).
-- Scene source10 is deliberately excluded: its base-object contract differs.
UPDATE conditions SET ConditionTarget=1
WHERE SourceTypeOrReferenceId=22 AND SourceId IN(0,1,9)
AND ConditionTypeOrReference=44 AND ConditionTarget=0;
