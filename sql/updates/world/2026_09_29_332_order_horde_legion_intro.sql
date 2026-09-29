-- Keep the regular Horde Legion introduction in reward order.
-- The level-100 ship tutorial still starts directly at quest 40518.
CREATE TEMPORARY TABLE `legion_intro_order_fix` (
    `position` INT PRIMARY KEY, `quest` INT, `previous` INT, `next` INT);
INSERT INTO `legion_intro_order_fix` VALUES
(0,43926,0,44281),(1,44281,43926,40518),(2,40518,44281,40522),
(3,40522,40518,40760),(4,40760,40522,40607),(5,40607,40760,40605),
(6,40605,40607,44663),(7,44663,40605,0);

UPDATE `quest_template_addon` a INNER JOIN `legion_intro_order_fix` s ON a.`ID`=s.`quest`
SET a.`PrevQuestID`=s.`previous`, a.`NextQuestID`=s.`next`
WHERE s.`quest`<>44663; -- shared with the Alliance and demon hunter routes
UPDATE `quest_template` q INNER JOIN `legion_intro_order_fix` s ON q.`ID`=s.`quest`
SET q.`RewardNextQuest`=s.`next`;

-- Offer only the next regular Horde stage. Merely completing a prerequisite
-- is insufficient: it must be turned in. An active/rewarded later stage also
-- prevents returning to an earlier quest.
DELETE c FROM `conditions` c INNER JOIN `legion_intro_order_fix` s ON c.`SourceEntry`=s.`quest`
WHERE c.`SourceTypeOrReferenceId`=19 AND c.`SourceGroup`=0 AND c.`SourceId`=0;
INSERT INTO `conditions`
(`SourceTypeOrReferenceId`,`SourceGroup`,`SourceEntry`,`SourceId`,`ElseGroup`,
 `ConditionTypeOrReference`,`ConditionTarget`,`ConditionValue1`,`ConditionValue2`,
 `ConditionValue3`,`NegativeCondition`,`ErrorTextId`,`ScriptName`,`Comment`)
SELECT 19,0,s.`quest`,0,1,6,0,67,0,0,0,0,'','Horde Legion intro - Horde route' FROM `legion_intro_order_fix` s;
INSERT INTO `conditions`
(`SourceTypeOrReferenceId`,`SourceGroup`,`SourceEntry`,`SourceId`,`ElseGroup`,
 `ConditionTypeOrReference`,`ConditionTarget`,`ConditionValue1`,`ConditionValue2`,
 `ConditionValue3`,`NegativeCondition`,`ErrorTextId`,`ScriptName`,`Comment`)
SELECT 19,0,s.`quest`,0,1,15,0,2048,0,0,1,0,'','Horde Legion intro - regular class route' FROM `legion_intro_order_fix` s;
INSERT INTO `conditions`
(`SourceTypeOrReferenceId`,`SourceGroup`,`SourceEntry`,`SourceId`,`ElseGroup`,
 `ConditionTypeOrReference`,`ConditionTarget`,`ConditionValue1`,`ConditionValue2`,
 `ConditionValue3`,`NegativeCondition`,`ErrorTextId`,`ScriptName`,`Comment`)
SELECT 19,0,s.`quest`,0,1,8,0,s.`previous`,0,0,0,0,'','Horde Legion intro - previous quest rewarded'
FROM `legion_intro_order_fix` s WHERE s.`previous`<>0;
-- MySQL cannot reference one temporary table twice in a SELECT.
CREATE TEMPORARY TABLE `legion_intro_later_fix` AS SELECT * FROM `legion_intro_order_fix`;
INSERT INTO `conditions`
(`SourceTypeOrReferenceId`,`SourceGroup`,`SourceEntry`,`SourceId`,`ElseGroup`,
 `ConditionTypeOrReference`,`ConditionTarget`,`ConditionValue1`,`ConditionValue2`,
 `ConditionValue3`,`NegativeCondition`,`ErrorTextId`,`ScriptName`,`Comment`)
SELECT 19,0,s.`quest`,0,1,14,0,l.`quest`,0,0,0,0,'','Horde Legion intro - no later quest already started'
FROM `legion_intro_order_fix` s INNER JOIN `legion_intro_later_fix` l ON l.`position`>s.`position`;
INSERT INTO `conditions`
(`SourceTypeOrReferenceId`,`SourceGroup`,`SourceEntry`,`SourceId`,`ElseGroup`,
 `ConditionTypeOrReference`,`ConditionTarget`,`ConditionValue1`,`ConditionValue2`,
 `ConditionValue3`,`NegativeCondition`,`ErrorTextId`,`ScriptName`,`Comment`)
VALUES
(19,0,44663,0,2,6,0,469,0,0,0,0,'','Legion intro - Alliance route to Dalaran'),
(19,0,44663,0,2,8,0,44120,0,0,0,0,'','Legion intro - Alliance handoff rewarded'),
(19,0,44663,0,3,15,0,2048,0,0,0,0,'','Legion intro - demon hunter route to Dalaran'),
(19,0,44663,0,3,8,0,41002,0,0,0,0,'','Legion intro - demon hunter handoff rewarded');

-- This realm now uses the full regular Horde introduction. Remove Holgar's
-- skip, which added 44663 and granted both objectives before the story.
DELETE FROM `smart_scripts` WHERE `entryorguid`=4311 AND `source_type`=0 AND (
(`id`=0 AND `event_type`=62 AND `event_param1`=20487 AND `event_param2`=0 AND `action_type`=7 AND `action_param1`=44663)
OR (`id`=1 AND `event_type`=61 AND `action_type`=33 AND `action_param1`=113762)
OR (`id`=2 AND `event_type`=61 AND `action_type`=33 AND `action_param1`=114506)
OR (`id`=3 AND `event_type`=61 AND `action_type`=85 AND `action_param1`=230156));
DELETE FROM `gossip_menu_option` WHERE `MenuID`=20487 AND `OptionID`=0 AND `OptionBroadcastTextID`=123132;
DELETE FROM `conditions` WHERE `SourceTypeOrReferenceId`=15 AND `SourceGroup`=20487 AND `SourceEntry`=0;

-- 7422 is the attack/turn-in phase. After reward, use the peaceful 6745
-- Sylvanas handoff until Keep Your Friends Close has been rewarded.
DELETE FROM `conditions` WHERE `SourceTypeOrReferenceId`=23 AND `SourceGroup`=14 AND `SourceEntry`=1 AND `ElseGroup`=4;
DELETE FROM `conditions` WHERE `SourceTypeOrReferenceId`=23 AND `SourceGroup`=14 AND `SourceEntry`=150;
INSERT INTO `conditions`
(`SourceTypeOrReferenceId`,`SourceGroup`,`SourceEntry`,`SourceId`,`ElseGroup`,
 `ConditionTypeOrReference`,`ConditionTarget`,`ConditionValue1`,`ConditionValue2`,
 `ConditionValue3`,`NegativeCondition`,`ErrorTextId`,`ScriptName`,`Comment`)
VALUES
(23,14,150,0,0,8,0,40607,0,0,0,0,'','Horde Legion intro - demon attack rewarded'),
(23,14,150,0,0,8,0,40605,0,0,1,0,'','Horde Legion intro - Illidari handoff not rewarded');
UPDATE `phase_definitions` SET `phasemask`=1 WHERE `zoneId`=14 AND `entry`=150 AND `phaseId`='6745';
UPDATE `creature` SET `phaseMask`=1 WHERE `id`=101035 AND `map`=1 AND `PhaseId`='6745';

-- The pre-teleport portal remains available for completed, unturned-in
-- Dalaran quests as well as incomplete ones.
DELETE FROM `conditions` WHERE `SourceTypeOrReferenceId`=23 AND `SourceGroup`=1637 AND `SourceEntry`=8 AND `ElseGroup`=1
    AND `Comment`='Legion intro - Dalaran quest complete, not yet rewarded';
INSERT INTO `conditions`
(`SourceTypeOrReferenceId`,`SourceGroup`,`SourceEntry`,`SourceId`,`ElseGroup`,
 `ConditionTypeOrReference`,`ConditionTarget`,`ConditionValue1`,`ConditionValue2`,
 `ConditionValue3`,`NegativeCondition`,`ErrorTextId`,`ScriptName`,`Comment`)
VALUES (23,1637,8,0,1,28,0,44663,0,0,0,0,'','Legion intro - Dalaran quest complete, not yet rewarded');

-- Keep the ordinary city visible together with the post-introduction portal.
UPDATE `phase_definitions` SET `phasemask`=3 WHERE `zoneId`=1637 AND `entry`=7 AND `phaseId`='7000';

-- Khadgar's ready option starts the teleport aura; there is no linked row 1.
UPDATE `smart_scripts` SET `link`=0 WHERE `entryorguid`=113986 AND `source_type`=0
    AND `id`=0 AND `link`=1 AND `event_type`=62 AND `event_param1`=20457
    AND `event_param2`=1 AND `action_type`=75 AND `action_param1`=227861;

-- 7.3.5 spell 228327 grants travel credit 113762 when the player enters old
-- Dalaran. Spell 227861 plays scene 1449 / package 1728 at Khadgar. Finish
-- that scene before teleporting to the Broken Isles and granting 114506.
-- SceneCompleted also handles the client's deliberate cinematic skip.
DELETE FROM `spell_scene_event` WHERE `MiscValue`=1449 AND `Event`='complete';
INSERT INTO `spell_scene_event` (`MiscValue`,`Event`,`trigerSpell`,`MonsterCredit`,`comment`)
VALUES (1449,'complete',230156,114506,'Legion intro - Dalaran teleport scene completed');

DROP TEMPORARY TABLE `legion_intro_later_fix`;
DROP TEMPORARY TABLE `legion_intro_order_fix`;
