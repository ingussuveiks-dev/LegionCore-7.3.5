-- The Battle for Gilneas City, map 654. Promote only with the matching server.
-- Keep the ordinary 70696 reward spell. Remove the legacy accept-time shortcut
-- and its event flag together, so only the native victory objective can finish.

START TRANSACTION;

SET @gilneas_battle_source = (
    EXISTS (SELECT 1 FROM `creature` WHERE `guid` = 149660 AND `id` = 38553 AND `map` = 654 AND `areaId` = 4842)
    AND EXISTS (SELECT 1 FROM `creature` WHERE `guid` = 149746 AND `id` = 38611 AND `map` = 654)
    AND EXISTS (SELECT 1 FROM `creature_template` WHERE `entry` = 38553 AND `AIName` = '' AND `ScriptName` = '')
    AND EXISTS (SELECT 1 FROM `gossip_menu_option` WHERE `MenuID` = 11061 AND `OptionID` = 0 AND `OptionBroadcastTextID` = 38484)
    AND EXISTS (SELECT 1 FROM `quest_template` WHERE `ID` = 24904 AND `StartItem` = 50334 AND `StartScript` = 24904 AND `CompleteScript` = 24904 AND `RewardNextQuest` = 24902)
    AND EXISTS (SELECT 1 FROM `quest_template_addon` WHERE `ID` = 24904 AND `PrevQuestID` = 24676 AND `NextQuestID` = 24902 AND `SpecialFlags` = 2)
    AND (SELECT COUNT(*) FROM `quest_objectives` WHERE `QuestID` = 24904) = 1
    AND EXISTS (SELECT 1 FROM `quest_objectives` WHERE `QuestID` = 24904 AND `ID` = 265467 AND `Type` = 0 AND `StorageIndex` = 0 AND `ObjectID` = 38854 AND `Amount` = 1 AND `Bugged` = 0)
    AND EXISTS (SELECT 1 FROM `quest_end_scripts` WHERE `id` = 24904 AND `delay` = 0 AND `command` = 15 AND `datalong` = 70696)
    AND NOT EXISTS (SELECT 1 FROM `areatrigger_questender` WHERE `quest` = 24904)
    AND NOT EXISTS (SELECT 1 FROM `spell_script_names` WHERE `spell_id` IN (72050,72113,72114,72116,72349,72361,72384))
    AND NOT EXISTS (SELECT 1 FROM `conditions` WHERE `SourceTypeOrReferenceId` = 13 AND `SourceEntry` IN (72114,72116,72361))
    AND (SELECT COUNT(*) FROM `quest_start_scripts` WHERE `id` = 24904 AND `delay` = 0 AND `command` = 7 AND `datalong` = 24904 AND `datalong2` = 0) = 1
    AND EXISTS (SELECT 1 FROM `quest_start_scripts` WHERE `id` = 24904 AND `delay` = 0 AND `command` = 6 AND `datalong` = 654
        AND ABS(`x` - (-1665.24)) < 0.01 AND ABS(`y` - 1613.95) < 0.01 AND ABS(`z` - 21.1747) < 0.01 AND ABS(`o` - 5.58549) < 0.01)
    AND NOT EXISTS (SELECT 1 FROM `quest_start_scripts` WHERE `id` = 24904 AND NOT (
        `delay` = 0 AND `dataint` = 0 AND `datalong2` = 0 AND (`command` = 7 AND `datalong` = 24904
        OR `command` = 6 AND `datalong` = 654 AND ABS(`x` - (-1665.24)) < 0.01 AND ABS(`y` - 1613.95) < 0.01
            AND ABS(`z` - 21.1747) < 0.01 AND ABS(`o` - 5.58549) < 0.01)))
);

-- Copy the matched localized broadcast and sound pairs from their existing
-- combined groups. Keep all original groups for any other consumers.
CREATE TEMPORARY TABLE `gilneas_battle_dialogue` LIKE `creature_text`;

INSERT INTO `gilneas_battle_dialogue`
    (`CreatureID`, `GroupID`, `ID`, `Text`, `Type`, `Language`, `Probability`, `Emote`, `Duration`, `Sound`, `BroadcastTextID`, `comment`)
SELECT dialogue.`Speaker`, dialogue.`NewGroup`, 0, MIN(texts.`Text`), MAX(texts.`Type`), 0, 100, MAX(texts.`Emote`), 0,
       dialogue.`Sound`, dialogue.`Broadcast`, 'Battle for Gilneas City - ordered dialogue'
FROM (
    SELECT 38218 AS `Speaker`, 30 AS `NewGroup`, 3 AS `OldGroup`, 19623 AS `Sound`, 37888 AS `Broadcast`
    UNION ALL SELECT 38218,31,3,19624,37893
    UNION ALL SELECT 38218,32,3,19625,37891
    UNION ALL SELECT 38218,33,3,19626,37892
    UNION ALL SELECT 38218,34,3,19627,37895
    UNION ALL SELECT 38218,35,3,19628,37896
    UNION ALL SELECT 38218,36,3,19651,37897
    UNION ALL SELECT 38218,37,3,19609,38362
    UNION ALL SELECT 38218,38,3,19610,38345
    UNION ALL SELECT 38218,39,3,19611,38360
    UNION ALL SELECT 38415,30,4,19499,38662
    UNION ALL SELECT 38415,31,3,19500,38361
    UNION ALL SELECT 38426,30,3,19684,38322
    UNION ALL SELECT 38469,30,4,23517,129681
    UNION ALL SELECT 38469,31,4,0,38368
    UNION ALL SELECT 38469,32,4,19727,38370
    UNION ALL SELECT 38470,30,3,19725,38364
    UNION ALL SELECT 38470,31,3,19726,38365
    UNION ALL SELECT 38474,30,3,19612,38369
    UNION ALL SELECT 38474,31,3,20562,38371
    UNION ALL SELECT 38474,32,3,20563,38372
) dialogue
JOIN `creature_text` texts ON texts.`CreatureID` = dialogue.`Speaker` AND texts.`GroupID` = dialogue.`OldGroup`
    AND texts.`Sound` = dialogue.`Sound` AND texts.`BroadcastTextID` = dialogue.`Broadcast`
WHERE @gilneas_battle_source
GROUP BY dialogue.`Speaker`, dialogue.`NewGroup`, dialogue.`Sound`, dialogue.`Broadcast`;

SET @gilneas_battle_ready = @gilneas_battle_source AND ROW_COUNT() = 21 AND NOT EXISTS (
    SELECT 1 FROM `gilneas_battle_dialogue` ready
    JOIN `creature_text` existing ON existing.`CreatureID` = ready.`CreatureID` AND existing.`GroupID` = ready.`GroupID`);

SET @gilneas_battle_installed = (
    EXISTS (SELECT 1 FROM `creature_template` WHERE `entry` = 38553 AND `ScriptName` = 'npc_gilneas_battle_krennan')
    AND EXISTS (SELECT 1 FROM `quest_template` WHERE `ID` = 24904 AND `StartScript` = 0 AND `CompleteScript` = 24904)
    AND EXISTS (SELECT 1 FROM `quest_template_addon` WHERE `ID` = 24904 AND (`SpecialFlags` & 2) = 0)
    AND NOT EXISTS (SELECT 1 FROM `quest_start_scripts` WHERE `id` = 24904)
    AND (SELECT COUNT(*) FROM `creature_text` WHERE `comment` = 'Battle for Gilneas City - ordered dialogue') = 21
    AND (SELECT COUNT(*) FROM `spell_script_names` WHERE (`spell_id`, `ScriptName`) IN (
        (72050,'spell_gilneas_battle_catapult'),(72113,'spell_gilneas_battle_wail'),
        (72114,'spell_gilneas_battle_prince'),(72116,'spell_gilneas_battle_prince'),(72361,'spell_gilneas_battle_prince'),
        (72349,'spell_gilneas_battle_victory'),(72384,'spell_gilneas_battle_despawn'))) = 7
    AND (SELECT COUNT(*) FROM `conditions` WHERE `SourceTypeOrReferenceId` = 13 AND `SourceGroup` = 1
        AND `SourceEntry` IN (72114,72116,72361) AND `ConditionTypeOrReference` = 31 AND `ConditionValue1` = 3 AND `ConditionValue2` = 38474) = 3
);
SET @gilneas_battle_guard = IF(@gilneas_battle_ready OR @gilneas_battle_installed, 'DO 0',
    'SIGNAL SQLSTATE ''45000'' SET MESSAGE_TEXT = ''Gilneas battle 24904 source or dialogue differs from verified data''');
PREPARE gilneas_battle_guard FROM @gilneas_battle_guard;
EXECUTE gilneas_battle_guard;
DEALLOCATE PREPARE gilneas_battle_guard;

INSERT INTO `creature_text` SELECT * FROM `gilneas_battle_dialogue` WHERE @gilneas_battle_ready;
DROP TEMPORARY TABLE `gilneas_battle_dialogue`;
UPDATE `creature_template` SET `ScriptName` = 'npc_gilneas_battle_krennan' WHERE @gilneas_battle_ready AND `entry` = 38553;
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`)
SELECT 72349, 'spell_gilneas_battle_victory' WHERE @gilneas_battle_ready
UNION ALL SELECT 72113, 'spell_gilneas_battle_wail' WHERE @gilneas_battle_ready
UNION ALL SELECT 72384, 'spell_gilneas_battle_despawn' WHERE @gilneas_battle_ready
UNION ALL SELECT 72114, 'spell_gilneas_battle_prince' WHERE @gilneas_battle_ready
UNION ALL SELECT 72116, 'spell_gilneas_battle_prince' WHERE @gilneas_battle_ready
UNION ALL SELECT 72361, 'spell_gilneas_battle_prince' WHERE @gilneas_battle_ready
UNION ALL SELECT 72050, 'spell_gilneas_battle_catapult' WHERE @gilneas_battle_ready;
INSERT INTO `conditions` (`SourceTypeOrReferenceId`, `SourceGroup`, `SourceEntry`, `ConditionTypeOrReference`, `ConditionValue1`, `ConditionValue2`, `Comment`)
SELECT 13, 1, 72114, 31, 3, 38474, 'Gilneas battle - candidate for the scoped finale aim' WHERE @gilneas_battle_ready
UNION ALL SELECT 13, 1, 72116, 31, 3, 38474, 'Gilneas battle - candidate for the scoped finale arrow' WHERE @gilneas_battle_ready
UNION ALL SELECT 13, 1, 72361, 31, 3, 38474, 'Gilneas battle - candidate for the scoped slain spell' WHERE @gilneas_battle_ready;
DELETE FROM `quest_start_scripts` WHERE @gilneas_battle_ready AND `id` = 24904;
UPDATE `quest_template` SET `StartScript` = 0 WHERE @gilneas_battle_ready AND `ID` = 24904;
UPDATE `quest_template_addon` SET `SpecialFlags` = `SpecialFlags` & ~2 WHERE @gilneas_battle_ready AND `ID` = 24904;

COMMIT;
