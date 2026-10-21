-- The map obtains this breadcrumb from QuestLineXQuest, independently of
-- Calydus's gossip option. Use the same unfinished-route/class predicate as
-- choice 245. Source 19 is evaluated by CanTakeQuest in UpdateAvailableQuestLines.
DELETE FROM conditions
 WHERE SourceTypeOrReferenceId=19 AND SourceGroup=0 AND SourceEntry=44089
 AND SourceId=0 AND Comment LIKE 'Warlock artifact map - %';
INSERT INTO conditions
 (SourceTypeOrReferenceId,SourceGroup,SourceEntry,SourceId,ElseGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2,ConditionValue3,NegativeCondition,Comment)
SELECT 19,0,44089,0,ElseGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2,ConditionValue3,NegativeCondition,
 CONCAT('Warlock artifact map - ',SUBSTRING(Comment,LENGTH('Warlock artifact choice - ')+1))
 FROM conditions
 WHERE SourceTypeOrReferenceId=32 AND SourceGroup=0 AND SourceEntry=245
 AND SourceId=0 AND Comment LIKE 'Warlock artifact choice - %';
