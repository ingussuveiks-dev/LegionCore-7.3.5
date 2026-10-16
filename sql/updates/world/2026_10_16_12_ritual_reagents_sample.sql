-- Ritual Reagents needs one sample. The stag's repeatable gossip previously
-- granted another while the player collected the other reagents.
-- Hide both collection choices once a sample exists (including the bank),
-- and guard the grant itself against repeated/stale selections.
DELETE FROM conditions
WHERE SourceTypeOrReferenceId=15 AND SourceGroup=19586 AND SourceEntry IN (0,1)
 AND SourceId=0 AND ElseGroup=0 AND ConditionTypeOrReference=2 AND ConditionValue1=136986;
DELETE FROM conditions
WHERE SourceTypeOrReferenceId=22 AND SourceGroup=1 AND SourceEntry=106581 AND SourceId=0
 AND ElseGroup=0 AND ((ConditionTypeOrReference=2 AND ConditionValue1=136986)
 OR (ConditionTypeOrReference=9 AND ConditionValue1=42128));
INSERT INTO conditions
 (SourceTypeOrReferenceId,SourceGroup,SourceEntry,SourceId,ElseGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2,ConditionValue3,NegativeCondition,Comment)
VALUES
 (15,19586,0,0,0,2,0,136986,1,1,1,'Ritual Reagents - Hide collection when sample already owned'),
 (15,19586,1,0,0,2,0,136986,1,1,1,'Ritual Reagents - Hide collection when sample already owned'),
 (22,1,106581,0,0,9,0,42128,0,0,0,'Ritual Reagents - Grant sample only during quest'),
 (22,1,106581,0,0,2,0,136986,1,1,1,'Ritual Reagents - Grant at most one sample');
