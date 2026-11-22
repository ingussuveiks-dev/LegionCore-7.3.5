-- The original slots contain the correct IDs with the opposite objective types.
UPDATE `quest_objectives` SET `Type` = 0 WHERE `QuestID` = 13975 AND `ID` = 264970 AND `ObjectID` = 65892 AND `Type` = 1;
UPDATE `quest_objectives` SET `Type` = 1 WHERE `QuestID` = 13975 AND `ID` = 269006 AND `ObjectID` = 46714 AND `Type` = 0;

-- Use the native spell and summon properties at the existing Rocco spawn point.
INSERT INTO `spell_target_position` (`id`, `target_map`, `target_position_x`, `target_position_y`, `target_position_z`, `target_orientation`)
SELECT 65662, 1, 207.45, -2940.07, 92.757, 1.633
WHERE NOT EXISTS (SELECT 1 FROM `spell_target_position` WHERE `id` = 65662);

UPDATE `creature_template`
SET `ScriptName` = CASE `entry` WHEN 34578 THEN 'npc_barrens_rocco_delivery' WHEN 34577 THEN 'npc_barrens_delivery_kodo' END
WHERE `entry` IN (34577,34578) AND `AIName` = 'SmartAI' AND `ScriptName` = '';

-- Keep the original dialogue, pack kodo, path, eject spell and Balgor outro.
UPDATE `smart_scripts`
SET `link` = 0, `action_type` = 1, `action_param1` = 0, `action_param2` = 5000, `action_param3` = 0,
    `target_type` = 1, `target_x` = 0, `target_y` = 0, `target_z` = 0, `target_o` = 0,
    `comment` = 'Rocco - after the real delivery summon - talk'
WHERE `entryorguid` = 34578 AND `source_type` = 0 AND `id` = 0 AND `action_type` = 12 AND `action_param1` = 34577;

DELETE FROM `smart_scripts` WHERE `entryorguid` = 34578 AND `source_type` = 0 AND `id` = 1 AND `action_type` = 1;
DELETE FROM `smart_scripts` WHERE `entryorguid` = 34577 AND `source_type` = 0 AND `id` = 2 AND `action_type` = 15 AND `action_param1` = 13975;
UPDATE `smart_scripts` SET `event_type` = 27, `target_type` = 7
WHERE `entryorguid` = 34577 AND `source_type` = 0 AND `id` = 1 AND `event_type` = 73 AND `action_type` = 53;
UPDATE `smart_scripts` SET `target_type` = 23, `target_param1` = 0, `target_param2` = 0
WHERE `entryorguid` = 34431 AND `source_type` = 0 AND `id` = 2 AND `action_type` = 45 AND `target_param1` = 34577;

INSERT INTO `conditions`
    (`SourceTypeOrReferenceId`, `SourceGroup`, `SourceEntry`, `SourceId`, `ElseGroup`, `ConditionTypeOrReference`,
     `ConditionTarget`, `ConditionValue1`, `ConditionValue2`, `ConditionValue3`, `NegativeCondition`, `ErrorTextId`, `ScriptName`, `Comment`)
SELECT r.`source`, r.`entry`, r.`option`, 0, 0, 9, 0, 13975, 0, 0, 0, 0, r.`script`, r.`description`
FROM (
    SELECT 15 AS `source`, 34578 AS `entry`, 0 AS `option`, '' AS `script`, 'Rocco - current caravan delivery quest' AS `description`
    UNION ALL
    SELECT 18, 34577, 46598, 'condition_barrens_delivery_rider', 'Head Caravan Kodo - current quest and actual rider'
) AS r
WHERE NOT EXISTS (
    SELECT 1 FROM `conditions` c
    WHERE c.`SourceTypeOrReferenceId` = r.`source` AND c.`SourceGroup` = r.`entry` AND c.`SourceEntry` = r.`option`
      AND c.`SourceId` = 0 AND c.`ElseGroup` = 0 AND c.`ConditionTypeOrReference` = 9 AND c.`ConditionTarget` = 0
      AND c.`ConditionValue1` = 13975 AND c.`ConditionValue2` = 0 AND c.`ConditionValue3` = 0
);

INSERT IGNORE INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
    (96791, 'spell_barrens_delivery_sweeper'),
    (96792, 'spell_barrens_delivery_sweeper');
