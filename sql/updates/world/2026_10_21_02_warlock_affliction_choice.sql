-- The artifact is already acquired when the Dark Riders acquisition quest
-- is complete/rewarded; a later return-to-Calydus quest need not be rewarded.
-- Both supported acquisition quests have Ariden/claim objectives 103473/100813.
-- Preserve the old return-quest checks and all other routes. No character
-- completion or ownership is granted/reset by this migration.
INSERT IGNORE INTO conditions
 (SourceTypeOrReferenceId,SourceGroup,SourceEntry,SourceId,ElseGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2,ConditionValue3,NegativeCondition,Comment)
VALUES
 (15,19326,0,0,0,8,0,40623,0,0,1,'Warlock artifact choice - route 523 acquisition not rewarded'),
 (32,0,245,0,0,8,0,40623,0,0,1,'Warlock artifact choice - route 523 acquisition not rewarded'),
 (33,0,523,0,0,8,0,40623,0,0,1,'Warlock artifact choice - route 523 acquisition not rewarded'),
 (15,19326,0,0,0,28,0,40623,0,0,1,'Warlock artifact choice - route 523 acquisition not awaiting turn-in'),
 (32,0,245,0,0,28,0,40623,0,0,1,'Warlock artifact choice - route 523 acquisition not awaiting turn-in'),
 (33,0,523,0,0,28,0,40623,0,0,1,'Warlock artifact choice - route 523 acquisition not awaiting turn-in'),
 (15,19326,0,0,0,8,0,41155,0,0,1,'Warlock artifact choice - route 523 acquisition not rewarded'),
 (32,0,245,0,0,8,0,41155,0,0,1,'Warlock artifact choice - route 523 acquisition not rewarded'),
 (33,0,523,0,0,8,0,41155,0,0,1,'Warlock artifact choice - route 523 acquisition not rewarded'),
 (15,19326,0,0,0,28,0,41155,0,0,1,'Warlock artifact choice - route 523 acquisition not awaiting turn-in'),
 (32,0,245,0,0,28,0,41155,0,0,1,'Warlock artifact choice - route 523 acquisition not awaiting turn-in'),
 (33,0,523,0,0,28,0,41155,0,0,1,'Warlock artifact choice - route 523 acquisition not awaiting turn-in');
