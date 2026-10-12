-- Rocs vs Eagles: one rescue per captive spawn; keep native release spell 188432.
UPDATE smart_scripts SET event_flags=1,link=2 WHERE entryorguid=94991 AND source_type=0 AND id=0;
-- Remove the captive aura and click flag immediately, before the timed departure.
UPDATE smart_scripts SET link=4 WHERE entryorguid=94991 AND source_type=0 AND id=1;
DELETE FROM smart_scripts WHERE entryorguid=94991 AND source_type=0 AND id IN(2,3,4);
INSERT INTO smart_scripts (entryorguid,source_type,id,link,event_type,event_chance,action_type,action_param1,target_type,comment) VALUES
(94991,0,2,3,61,100,28,188434,1,'Captive Great Eagle - released - remove captive aura'),
(94991,0,3,0,61,100,83,16777216,1,'Captive Great Eagle - released - disable spellclick'),
(94991,0,4,0,61,100,82,16777216,1,'Captive Great Eagle - reset - restore spellclick');

-- MOVE_Z consumes X,Y,Z offsets in action parameters 1,2,3 respectively.
-- Existing 15,0,0 moved sideways at ground height; use the same distance upward.
UPDATE smart_scripts SET action_param1=0,action_param2=0,action_param3=15
WHERE entryorguid=9499100 AND source_type=9 AND id=1 AND action_type=202;

DELETE FROM conditions WHERE (SourceTypeOrReferenceId=18 AND SourceGroup=94991 AND SourceEntry=188432)
 OR (SourceTypeOrReferenceId=22 AND SourceGroup=1 AND SourceEntry=94991 AND SourceId=0);
INSERT INTO conditions (SourceTypeOrReferenceId,SourceGroup,SourceEntry,SourceId,ConditionTypeOrReference,ConditionTarget,ConditionValue1,Comment) VALUES
(18,94991,188432,0,9,0,38910,'Captive Great Eagle - only active Rocs vs Eagles players can click'),
(22,1,94991,0,9,0,38910,'Captive Great Eagle - release hit requires active Rocs vs Eagles on caster');
