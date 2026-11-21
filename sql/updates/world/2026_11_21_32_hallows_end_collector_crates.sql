-- Hallow's End uses the original questgiver objects and the existing two return recipients per faction.
INSERT INTO `gameobject` (`guid`, `id`, `map`, `position_x`, `position_y`, `position_z`, `orientation`,
    `rotation2`, `rotation3`, `spawntimesecs`, `animprogress`, `state`) VALUES
(202611270, 209072, 0, -8502.49, 763.258, 72.0278, 2.23578, 0.899178, 0.437583, 300, 255, 1),
(202611271, 209094, 0, 1583.24, 45.5973, 61.6702, 1.56759, 0.705972, 0.70824, 300, 255, 1),
(202611272, 209076, 0, -8368.35, 602.336, 97.7787, 3.0408, 0.99873, 0.0503767, 300, 255, 1),
(202611273, 209076, 0, -8857.57, 672.989, 97.904, 3.71576, 0.959073, -0.283159, 300, 255, 1),
(202611274, 209095, 1, 1911.62, -4755.3, 38.9285, 2.14334, 0.878001, 0.478659, 300, 255, 1),
(202611275, 209095, 1, 1587.62, -4457.38, 16.0513, 2.79914, 0.985377, 0.170389, 300, 255, 1);

INSERT INTO `game_event_gameobject` (`eventEntry`, `guid`)
SELECT 12, `guid` FROM `gameobject` WHERE `guid` BETWEEN 202611270 AND 202611275
AND `id` IN (209072, 209094, 209076, 209095);

UPDATE `gameobject_template` SET `ScriptName` = 'go_hallows_end_crate'
WHERE `entry` IN (209072, 209094, 209076, 209095) AND `type` = 2 AND `ScriptName` = '';

INSERT INTO `creature` (`guid`, `id`, `map`, `position_x`, `position_y`, `position_z`, `orientation`) VALUES
(202611270, 54072, 0, -8499, 760, 72.0278, 2.45),
(202611271, 54072, 0, 1586.2, 48.6, 61.6702, 4.10),
(202611272, 54118, 0, -8367.81, 603.988, 97.7791, 2.93477),
(202611273, 54170, 1, 1914.9, -4752.6, 38.9294, 2.75988),
(202611274, 54205, 0, -8501.05, 760.65, 72.0278, 3.24),
(202611275, 54205, 0, 1584.4, 47.85, 61.6702, 1.60);

INSERT INTO `game_event_creature` (`eventEntry`, `guid`) VALUES
(12, 202611270), (12, 202611271), (12, 202611272), (12, 202611273), (12, 202611274), (12, 202611275);

INSERT INTO `creature_addon` (`guid`, `bytes1`) VALUES (202611274, 7), (202611275, 7);

UPDATE `creature_template` SET `ScriptName` = 'npc_hallows_end_sanath'
WHERE `entry` = 54072 AND `ScriptName` = '';
UPDATE `creature_template` SET `ScriptName` = 'npc_hallows_end_unleashed_void', `faction` = 14,
    `minlevel` = 10, `maxlevel` = 10 WHERE `entry` = 54114 AND `ScriptName` = '';

INSERT INTO `creature_template_scaling` (`Entry`, `LevelScalingMin`, `LevelScalingMax`, `LevelScalingDuration`, `VerifiedBuild`)
VALUES (54114, 10, 110, 100, 26972);

INSERT INTO `creature_text` (`CreatureID`, `GroupID`, `ID`, `Text`, `Type`, `Probability`, `comment`) VALUES
(54072, 0, 0, 'An intruder? Let us see what is inside this crate.', 12, 100, 'Sanath Lim-yo - unleashed void');

-- The final object is both the offer and the completion source for the native auto-complete quest.
INSERT INTO `gameobject_queststarter` (`id`, `quest`) VALUES (209076, 29413), (209095, 29429);
UPDATE `quest_template_addon` SET `PrevQuestID` = 29411 WHERE `ID` = 29413 AND `PrevQuestID` = 0;
UPDATE `quest_template_addon` SET `PrevQuestID` = 29428 WHERE `ID` = 29429 AND `PrevQuestID` = 0;

INSERT INTO `conditions` (`SourceTypeOrReferenceId`, `SourceEntry`, `ConditionTypeOrReference`, `ConditionValue1`, `Comment`) VALUES
(19, 29413, 12, 12, 'The Creepy Crate - Hallow''s End'),
(19, 29429, 12, 12, 'The Creepy Crate - Hallow''s End');
