-- These conversations are EventObject SmartScripts (13), not scenes (10).
DELETE current FROM conditions current JOIN conditions old
 ON current.SourceTypeOrReferenceId=old.SourceTypeOrReferenceId
 AND current.SourceGroup=old.SourceGroup AND current.SourceEntry=old.SourceEntry
 AND current.ElseGroup=old.ElseGroup
 AND current.ConditionTypeOrReference=old.ConditionTypeOrReference
 AND current.ConditionTarget=old.ConditionTarget
 AND current.ConditionValue1=old.ConditionValue1 AND current.ConditionValue2=old.ConditionValue2
 AND current.ConditionValue3=old.ConditionValue3 AND current.NegativeCondition=old.NegativeCondition
 WHERE current.SourceTypeOrReferenceId=22 AND current.SourceId=13 AND old.SourceId=10
 AND current.SourceGroup=1 AND current.SourceEntry IN (309,368);
UPDATE conditions SET SourceId=13
 WHERE SourceTypeOrReferenceId=22 AND SourceId=10 AND SourceGroup=1 AND SourceEntry IN (309,368);
DELETE FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceId=13 AND SourceGroup=1
 AND SourceEntry=368 AND ConditionTypeOrReference=9 AND ConditionValue1=45572;
INSERT INTO conditions
 (SourceTypeOrReferenceId,SourceGroup,SourceEntry,SourceId,ElseGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1,Comment)
VALUES (22,1,368,13,0,9,0,45572,'Holding Our Ground - conversation requires active quest');

-- Spellclick visibility and server admission must match the native quest.
DELETE FROM conditions WHERE SourceTypeOrReferenceId=18
 AND (SourceGroup,SourceEntry) IN ((107988,214537),(108081,46598),(117453,233844),(117520,233844));
INSERT INTO conditions
 (SourceTypeOrReferenceId,SourceGroup,SourceEntry,SourceId,ElseGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2,ConditionValue3,NegativeCondition,ErrorTextId,ScriptName,Comment)
VALUES
 (18,107988,214537,0,0,9,0,42630,0,0,0,0,'','Bolas Bastion - launcher only during quest'),
 (18,108081,46598,0,0,9,0,42630,0,0,0,0,'','Bolas Bastion - personal vehicle boarding only during quest'),
 (18,117453,233844,0,0,9,0,45572,0,0,0,0,'','Holding Our Ground - rescue only during quest'),
 (18,117520,233844,0,0,9,0,45572,0,0,0,0,'','Holding Our Ground - rescue only during quest'),
 (18,117453,233844,0,0,41,0,45572,117454,3,1,0,'','Holding Our Ground - stop after three rescues'),
 (18,117520,233844,0,0,41,0,45572,117454,3,1,0,'','Holding Our Ground - stop after three rescues');

-- Validate the actual rescue hit as well, before its despawn action list.
DELETE FROM conditions WHERE SourceTypeOrReferenceId=22 AND SourceId=0
 AND SourceEntry IN (117453,117520) AND SourceGroup=1
 AND ConditionTypeOrReference IN (9,41);
INSERT INTO conditions
 (SourceTypeOrReferenceId,SourceGroup,SourceEntry,SourceId,ElseGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2,ConditionValue3,NegativeCondition,ErrorTextId,ScriptName,Comment)
VALUES
 (22,1,117453,0,0,9,0,45572,0,0,0,0,'','Holding Our Ground - validate rescue spell hit'),
 (22,1,117520,0,0,9,0,45572,0,0,0,0,'','Holding Our Ground - validate rescue spell hit'),
 (22,1,117453,0,0,41,0,45572,117454,3,1,0,'','Holding Our Ground - no repeated rescue credit'),
 (22,1,117520,0,0,41,0,45572,117454,3,1,0,'','Holding Our Ground - no repeated rescue credit');
