-- 7.3.5 QuestLineXQuest 93: The Skyhorn Tribe -> Nursing the Wounds -> Rocs vs Eagles.
-- The Bloodtotem return (40229) is a different branch.
UPDATE quest_template_addon SET NextQuestID=39318 WHERE ID=38913 AND NextQuestID=40515;
UPDATE quest_template_addon SET PrevQuestID=38913 WHERE ID=39318 AND PrevQuestID=40229;

-- Totem of Winds grants native override bar 603 / Suppressing Wind 189168.
-- Status mask 66 allowed COMPLETE/REWARDED, excluding INCOMPLETE (bit 3).
-- Keep the manually acquired ability during the quest; remove on abandon/reward.
UPDATE spell_area SET quest_end_status=10 WHERE spell=189182 AND area=7726 AND quest_end=39318;

DELETE FROM conditions WHERE SourceTypeOrReferenceId=15 AND SourceGroup=18894 AND SourceEntry=0;
INSERT INTO conditions (SourceTypeOrReferenceId,SourceGroup,SourceEntry,ConditionTypeOrReference,ConditionValue1,Comment) VALUES
(15,18894,0,9,39318,'Windmaster Julan: take the totem only while Nursing the Wounds is incomplete');
