-- Legion QuestLineXQuest line 573 starts with the class-specific Iron Bough lesson.
-- Leave the old turn-in and RewardNextQuest links usable for existing characters.
CREATE TEMPORARY TABLE `wandering_isle_first_lesson` (
    `old_quest` MEDIUMINT UNSIGNED PRIMARY KEY,
    `lesson` MEDIUMINT UNSIGNED NOT NULL UNIQUE
);
INSERT INTO `wandering_isle_first_lesson` VALUES
    (30039,30027),(30040,30033),(30041,30034),(30042,30035),
    (30043,30036),(30044,30037),(30045,30038);

START TRANSACTION;

-- Match the existing academy invisibility rule on the Legion lesson as well.
-- Preserve the existing rule for characters with the earlier quest in progress.
INSERT INTO `spell_area`
    (`spell`,`area`,`quest_start`,`quest_end`,`aura_spell`,`racemask`,`classmask`,
     `active_event`,`gender`,`autocast`,`quest_start_status`,`quest_end_status`)
SELECT old.`spell`,old.`area`,pair.`lesson`,old.`quest_end`,old.`aura_spell`,old.`racemask`,old.`classmask`,
       old.`active_event`,old.`gender`,old.`autocast`,old.`quest_start_status`,old.`quest_end_status`
FROM `wandering_isle_first_lesson` AS pair
JOIN `spell_area` AS old ON old.`spell`=56267 AND old.`area`=5834 AND old.`quest_start`=pair.`old_quest`
    AND old.`quest_end`=29409 AND old.`aura_spell`=0 AND old.`racemask`=0 AND old.`classmask`=0
    AND old.`active_event`=0 AND old.`gender`=2 AND old.`autocast`=1
    AND old.`quest_start_status`=74 AND old.`quest_end_status`=8
JOIN `creature_queststarter` AS starter ON starter.`id`=53566 AND starter.`quest`=pair.`lesson`
LEFT JOIN `spell_area` AS new ON new.`spell`=56267 AND new.`area`=5834 AND new.`quest_start`=pair.`lesson`
    AND new.`aura_spell`=0 AND new.`racemask`=0 AND new.`classmask`=0 AND new.`active_event`=0 AND new.`gender`=2
WHERE new.`spell` IS NULL;

-- Both addon columns contribute prerequisites to the same quest.
UPDATE `quest_template_addon` AS intro
JOIN `wandering_isle_first_lesson` AS pair ON pair.`old_quest`=intro.`ID` AND pair.`lesson`=intro.`NextQuestID`
JOIN `quest_template_addon` AS lesson ON lesson.`ID`=pair.`lesson`
JOIN `creature_queststarter` AS starter ON starter.`id`=53566 AND starter.`quest`=pair.`lesson`
SET intro.`NextQuestID`=0, lesson.`PrevQuestID`=0
WHERE lesson.`PrevQuestID` IN (0,pair.`old_quest`);

DELETE area FROM `area_queststart` AS area
JOIN `wandering_isle_first_lesson` AS pair ON pair.`old_quest`=area.`quest`
JOIN `quest_template_addon` AS intro ON intro.`ID`=pair.`old_quest` AND intro.`NextQuestID`=0
JOIN `quest_template_addon` AS lesson ON lesson.`ID`=pair.`lesson` AND lesson.`PrevQuestID`=0
JOIN `creature_queststarter` AS starter ON starter.`id`=53566 AND starter.`quest`=pair.`lesson`
WHERE area.`id`=5834;

COMMIT;
DROP TEMPORARY TABLE `wandering_isle_first_lesson`;
