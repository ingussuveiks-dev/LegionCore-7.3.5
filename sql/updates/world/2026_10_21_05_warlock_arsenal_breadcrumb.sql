-- Native QuestLine 274 orders 40684 -> 43984 -> 44089. The final selection
-- had an ender but no starter, so gossip could bypass it while the map offered it.
INSERT IGNORE INTO creature_queststarter (id,quest) VALUES (101097,44089);
UPDATE quest_template_addon SET PrevQuestID=40684 WHERE ID=43984 AND PrevQuestID=0;
UPDATE quest_template_addon SET PrevQuestID=43984 WHERE ID=44089 AND PrevQuestID=0;

-- Also retire the second-weapon breadcrumb if all acquisition routes finished.
DELETE FROM conditions WHERE SourceTypeOrReferenceId=19 AND SourceGroup=0
 AND SourceEntry=43984 AND SourceId=0 AND Comment LIKE 'Warlock arsenal breadcrumb - %';
INSERT INTO conditions
 (SourceTypeOrReferenceId,SourceGroup,SourceEntry,SourceId,ElseGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2,ConditionValue3,NegativeCondition,Comment)
SELECT 19,0,43984,0,ElseGroup,ConditionTypeOrReference,ConditionTarget,ConditionValue1,ConditionValue2,ConditionValue3,NegativeCondition,
 CONCAT('Warlock arsenal breadcrumb - ',SUBSTRING(Comment,LENGTH('Warlock artifact choice - ')+1))
 FROM conditions WHERE SourceTypeOrReferenceId=32 AND SourceGroup=0 AND SourceEntry=245
 AND SourceId=0 AND Comment LIKE 'Warlock artifact choice - %';

-- Open the same native artifact choice as 40684, with the accepting Player
-- as caster and target. Do not grant or reward these quests for existing players.
DELETE FROM smart_scripts WHERE entryorguid=101097 AND source_type=0 AND id IN(30,31)
 AND comment LIKE 'Warlock arsenal breadcrumb - %';
INSERT IGNORE INTO smart_scripts
 (entryorguid,source_type,id,event_type,event_param1,action_type,action_param1,target_type,comment)
VALUES
 (101097,0,30,19,43984,85,199411,7,'Warlock arsenal breadcrumb - second artifact accept opens choice'),
 (101097,0,31,19,44089,85,199411,7,'Warlock arsenal breadcrumb - final artifact accept opens choice');
