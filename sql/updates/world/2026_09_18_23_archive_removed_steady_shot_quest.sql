-- Orc hunter training quest Steady Shot (25139) and spell 56641 were removed
-- in patch 7.0.3. Archive every active row for this quest, detach its immediate
-- predecessor, and remove the obsolete quest from the live 7.3.5 relations.
CREATE TABLE IF NOT EXISTS `_backup_20260918_obsolete_steady_shot_template`
LIKE `quest_template`;
CREATE TABLE IF NOT EXISTS `_backup_20260918_obsolete_steady_shot_addon`
LIKE `quest_template_addon`;
CREATE TABLE IF NOT EXISTS `_backup_20260918_obsolete_steady_shot_locale`
LIKE `quest_template_locale`;
CREATE TABLE IF NOT EXISTS `_backup_20260918_obsolete_steady_shot_objectives`
LIKE `quest_objectives`;
CREATE TABLE IF NOT EXISTS `_backup_20260918_obsolete_steady_shot_objective_locale`
LIKE `quest_objectives_locale`;
CREATE TABLE IF NOT EXISTS `_backup_20260918_obsolete_steady_shot_offer`
LIKE `quest_offer_reward`;
CREATE TABLE IF NOT EXISTS `_backup_20260918_obsolete_steady_shot_offer_locale`
LIKE `quest_offer_reward_locale`;
CREATE TABLE IF NOT EXISTS `_backup_20260918_obsolete_steady_shot_starter`
LIKE `creature_queststarter`;
CREATE TABLE IF NOT EXISTS `_backup_20260918_obsolete_steady_shot_ender`
LIKE `creature_questender`;

REPLACE INTO `_backup_20260918_obsolete_steady_shot_template`
SELECT * FROM `quest_template`
WHERE `ID` = 25139 OR (`ID` = 3087 AND `RewardNextQuest` = 25139);
REPLACE INTO `_backup_20260918_obsolete_steady_shot_addon`
SELECT * FROM `quest_template_addon`
WHERE `ID` = 25139 OR (`ID` = 3087 AND `NextQuestID` = 25139);
REPLACE INTO `_backup_20260918_obsolete_steady_shot_locale`
SELECT * FROM `quest_template_locale` WHERE `ID` = 25139;
REPLACE INTO `_backup_20260918_obsolete_steady_shot_objectives`
SELECT * FROM `quest_objectives` WHERE `QuestID` = 25139;
REPLACE INTO `_backup_20260918_obsolete_steady_shot_objective_locale`
SELECT * FROM `quest_objectives_locale` WHERE `QuestId` = 25139;
REPLACE INTO `_backup_20260918_obsolete_steady_shot_offer`
SELECT * FROM `quest_offer_reward` WHERE `ID` = 25139;
REPLACE INTO `_backup_20260918_obsolete_steady_shot_offer_locale`
SELECT * FROM `quest_offer_reward_locale` WHERE `ID` = 25139;
REPLACE INTO `_backup_20260918_obsolete_steady_shot_starter`
SELECT * FROM `creature_queststarter` WHERE `quest` = 25139;
REPLACE INTO `_backup_20260918_obsolete_steady_shot_ender`
SELECT * FROM `creature_questender` WHERE `quest` = 25139;

UPDATE `quest_template_addon`
SET `NextQuestID` = 0
WHERE `ID` = 3087
  AND `NextQuestID` = 25139;

UPDATE `quest_template`
SET `RewardNextQuest` = 0
WHERE `ID` = 3087
  AND `RewardNextQuest` = 25139;

DELETE FROM `creature_queststarter` WHERE `quest` = 25139;
DELETE FROM `creature_questender` WHERE `quest` = 25139;
DELETE FROM `quest_objectives_locale` WHERE `QuestId` = 25139;
DELETE FROM `quest_objectives` WHERE `QuestID` = 25139;
DELETE FROM `quest_offer_reward_locale` WHERE `ID` = 25139;
DELETE FROM `quest_offer_reward` WHERE `ID` = 25139;
DELETE FROM `quest_template_locale` WHERE `ID` = 25139;
DELETE FROM `quest_template_addon` WHERE `ID` = 25139;
DELETE FROM `quest_template` WHERE `ID` = 25139;
