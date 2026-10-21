-- Death/progression now belongs to the instance, keyed by distinct DB spawn.
-- Preserve the Dweller's Tormenting Eye and unrelated events.
DELETE FROM smart_scripts WHERE entryorguid=101991 AND source_type=0 AND event_type=6 AND action_type=11 AND action_param1=204852;
-- Awakening Roots uses native nearby-entry target 38, even when an explicit
-- boss target is supplied. Resolve it to Oakheart rather than a nearby player.
DELETE FROM conditions WHERE SourceTypeOrReferenceId=13 AND SourceGroup=1 AND SourceEntry=204852 AND SourceId=0 AND ElseGroup=0 AND ConditionTypeOrReference=31 AND ConditionTarget=0;
INSERT INTO conditions (SourceTypeOrReferenceId,SourceGroup,SourceEntry,SourceId,ElseGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2,ConditionValue3,NegativeCondition,Comment)
 VALUES (13,1,204852,0,0,31,0,3,103344,0,0,'Awakening Roots - native nearby target is Oakheart');
