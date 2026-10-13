-- Native 7.3.5: 199338 -> OverrideSpellData 643 -> Enslave Beshtal 199336.
-- This is a self-target aura. INVOKER_CAST can use a stale/non-player invoker
-- after Ritssyn's dialogue; force each selected player to cast on themselves.
UPDATE smart_scripts SET action_type=11,action_param2=18,comment='New Blood - grant native Enslave Beshtal button to players' WHERE entryorguid=98771 AND source_type=9 AND id=7 AND action_param1=199338;

-- Recover a missing button (including reconnects) only before Beshtal is enslaved.
-- Source 22 group is event id + 1; target 0 is the player seen by Ritssyn.
DELETE FROM smart_scripts WHERE entryorguid=98771 AND source_type=0 AND id=8;
INSERT INTO smart_scripts (entryorguid,source_type,id,event_type,event_chance,event_flags,event_param1,event_param2,event_param3,event_param4,action_type,action_param1,action_param2,target_type,comment) VALUES
(98771,0,8,10,100,0,1,30,1000,1000,11,199338,18,7,'New Blood - restore missing Enslave Beshtal button in cage');
DELETE FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceEntry=98771 AND SourceGroup=9 AND SourceId=0;
INSERT INTO conditions (SourceTypeOrReferenceId,SourceGroup,SourceEntry,SourceId,ElseGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2,ConditionValue3,NegativeCondition,Comment) VALUES
(22,9,98771,0,0,9,0,40729,0,0,0,'New Blood - active quest'),
(22,9,98771,0,0,44,0,962,1,0,0,'New Blood - Dread Warden stage'),
(22,9,98771,0,0,52,0,46006,0,0,1,'New Blood - Beshtal not yet enslaved'),
(22,9,98771,0,0,1,0,199338,0,0,1,'New Blood - button missing');

-- Do not permit replaying the enslavement after the cage has been opened.
DELETE FROM conditions WHERE SourceTypeOrReferenceId=17 AND SourceEntry=199336 AND SourceGroup=0 AND SourceId=0 AND ConditionTypeOrReference=52;
INSERT INTO conditions (SourceTypeOrReferenceId,SourceGroup,SourceEntry,SourceId,ElseGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1,NegativeCondition,Comment) VALUES
(17,0,199336,0,0,52,0,46006,1,'New Blood - enslave Beshtal only once');
