-- Calydus must stop offering completed acquisition routes. Hidden Chosen
-- quests deliberately do not gate this: abandoned routes must remain retryable.
-- Use final quest history, not item ownership (shop artifacts do not complete
-- their story). COMPLETE also covers a final quest awaiting Calydus turn-in.
DELETE FROM conditions
 WHERE Comment LIKE 'Warlock artifact choice - %'
 AND ((SourceTypeOrReferenceId=15 AND SourceGroup=19326 AND SourceEntry=0 AND SourceId=0)
   OR (SourceTypeOrReferenceId=32 AND SourceGroup=0 AND SourceEntry=245 AND SourceId=0)
   OR (SourceTypeOrReferenceId=33 AND SourceGroup=0 AND SourceEntry IN(523,524,525) AND SourceId=0));
DELETE FROM conditions
 WHERE SourceTypeOrReferenceId=15 AND SourceGroup=19326 AND SourceEntry=0
 AND SourceId=0 AND ElseGroup=0 AND ConditionTypeOrReference=8 AND ConditionTarget=0
 AND ConditionValue1=40684 AND ConditionValue2=0 AND ConditionValue3=0 AND NegativeCondition=0;
INSERT INTO conditions
 (SourceTypeOrReferenceId,SourceGroup,SourceEntry,SourceId,ElseGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2,ConditionValue3,NegativeCondition,Comment)
VALUES
 (15,19326,0,0,0,8,0,40684,0,0,0,'Warlock artifact choice - Calydus introduction rewarded'),
 (32,0,245,0,0,15,0,256,0,0,0,'Warlock artifact choice - Warlock only'),
 (15,19326,0,0,0,8,0,40712,0,0,1,'Warlock artifact choice - route 523 not rewarded'),
 (32,0,245,0,0,8,0,40712,0,0,1,'Warlock artifact choice - route 523 not rewarded'),
 (33,0,523,0,0,8,0,40712,0,0,1,'Warlock artifact choice - route 523 not rewarded'),
 (15,19326,0,0,0,28,0,40712,0,0,1,'Warlock artifact choice - route 523 not awaiting turn-in'),
 (32,0,245,0,0,28,0,40712,0,0,1,'Warlock artifact choice - route 523 not awaiting turn-in'),
 (33,0,523,0,0,28,0,40712,0,0,1,'Warlock artifact choice - route 523 not awaiting turn-in'),
 (15,19326,0,0,0,8,0,41156,0,0,1,'Warlock artifact choice - route 523 not rewarded'),
 (32,0,245,0,0,8,0,41156,0,0,1,'Warlock artifact choice - route 523 not rewarded'),
 (33,0,523,0,0,8,0,41156,0,0,1,'Warlock artifact choice - route 523 not rewarded'),
 (15,19326,0,0,0,28,0,41156,0,0,1,'Warlock artifact choice - route 523 not awaiting turn-in'),
 (32,0,245,0,0,28,0,41156,0,0,1,'Warlock artifact choice - route 523 not awaiting turn-in'),
 (33,0,523,0,0,28,0,41156,0,0,1,'Warlock artifact choice - route 523 not awaiting turn-in'),
 (15,19326,0,0,1,8,0,40684,0,0,0,'Warlock artifact choice - Calydus introduction rewarded'),
 (32,0,245,0,1,15,0,256,0,0,0,'Warlock artifact choice - Warlock only'),
 (15,19326,0,0,1,8,0,42125,0,0,1,'Warlock artifact choice - route 524 not rewarded'),
 (32,0,245,0,1,8,0,42125,0,0,1,'Warlock artifact choice - route 524 not rewarded'),
 (33,0,524,0,0,8,0,42125,0,0,1,'Warlock artifact choice - route 524 not rewarded'),
 (15,19326,0,0,1,28,0,42125,0,0,1,'Warlock artifact choice - route 524 not awaiting turn-in'),
 (32,0,245,0,1,28,0,42125,0,0,1,'Warlock artifact choice - route 524 not awaiting turn-in'),
 (33,0,524,0,0,28,0,42125,0,0,1,'Warlock artifact choice - route 524 not awaiting turn-in'),
 (15,19326,0,0,2,8,0,40684,0,0,0,'Warlock artifact choice - Calydus introduction rewarded'),
 (32,0,245,0,2,15,0,256,0,0,0,'Warlock artifact choice - Warlock only'),
 (15,19326,0,0,2,8,0,43254,0,0,1,'Warlock artifact choice - route 525 not rewarded'),
 (32,0,245,0,2,8,0,43254,0,0,1,'Warlock artifact choice - route 525 not rewarded'),
 (33,0,525,0,0,8,0,43254,0,0,1,'Warlock artifact choice - route 525 not rewarded'),
 (15,19326,0,0,2,28,0,43254,0,0,1,'Warlock artifact choice - route 525 not awaiting turn-in'),
 (32,0,245,0,2,28,0,43254,0,0,1,'Warlock artifact choice - route 525 not awaiting turn-in'),
 (33,0,525,0,0,28,0,43254,0,0,1,'Warlock artifact choice - route 525 not awaiting turn-in');
