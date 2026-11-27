-- Isolated opening fixture. The production cutover also needs the remaining
-- encounters and the real victory, so this file stays outside the updater.

UPDATE `creature_template`
SET `ScriptName` = 'npc_gilneas_battle_krennan'
WHERE `entry` = 38553 AND `AIName` = '' AND `ScriptName` = '';

-- The recorded opening lines appear among many other Liam yells in group 3.
-- Copy the seven matched voice/text pairs into ordered groups for the opening.
INSERT IGNORE INTO `creature_text`
    (`CreatureID`, `GroupID`, `ID`, `Text`, `Type`, `Language`, `Probability`, `Emote`, `Duration`, `Sound`, `BroadcastTextID`, `comment`)
SELECT 38218, speech.`GroupID`, 0, MIN(texts.`Text`), 14, 0, 100, MAX(texts.`Emote`), 0,
       speech.`Sound`, speech.`BroadcastTextID`, 'Prince Liam Greymane - Battle for Gilneas City opening'
FROM (
    SELECT 30 AS `GroupID`, 19623 AS `Sound`, 37888 AS `BroadcastTextID`
    UNION ALL SELECT 31, 19624, 37893
    UNION ALL SELECT 32, 19625, 37891
    UNION ALL SELECT 33, 19626, 37892
    UNION ALL SELECT 34, 19627, 37895
    UNION ALL SELECT 35, 19628, 37896
    UNION ALL SELECT 36, 19651, 37897
) speech
JOIN `creature_text` texts ON texts.`CreatureID` = 38218 AND texts.`GroupID` = 3
    AND texts.`Sound` = speech.`Sound` AND texts.`BroadcastTextID` = speech.`BroadcastTextID`
GROUP BY speech.`GroupID`, speech.`Sound`, speech.`BroadcastTextID`;
