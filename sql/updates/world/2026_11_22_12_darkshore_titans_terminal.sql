-- The native detector has a beach focus and a dummy area-entry effect. Only
-- actual buried-treasure markers may receive that effect.
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`)
VALUES (65300, 'spell_darkshore_buried_artifact_detector')
ON DUPLICATE KEY UPDATE `ScriptName` = VALUES(`ScriptName`);

INSERT INTO `conditions`
    (`SourceTypeOrReferenceId`, `SourceGroup`, `SourceEntry`, `SourceId`, `ElseGroup`, `ConditionTypeOrReference`,
     `ConditionTarget`, `ConditionValue1`, `ConditionValue2`, `ConditionValue3`, `NegativeCondition`, `ErrorTextId`, `ScriptName`, `Comment`)
SELECT rules.`source`, rules.`group_id`, rules.`entry`, 0, 0, rules.`type`, 0, rules.`v1`, rules.`v2`, 0, rules.`negative`, 0, '',
       'The Titans Terminal - native beach marker, active quest and carried materials'
FROM (
    SELECT 13 AS `source`, 1 AS `group_id`, 65300 AS `entry`, 31 AS `type`, 3 AS `v1`, 34356 AS `v2`, 0 AS `negative`
    UNION ALL SELECT 17, 0, 65300, 9, 13918, 0, 0
    UNION ALL SELECT 17, 0, 65300, 2, 46388, 1, 0
    UNION ALL SELECT 17, 0, 65435, 9, 13918, 0, 0
    UNION ALL SELECT 17, 0, 65435, 2, 46702, 5, 0
    UNION ALL SELECT 4, 195055, 46702, 9, 13918, 0, 0
    UNION ALL SELECT 4, 195055, 46702, 2, 46702, 5, 1
) AS rules
WHERE NOT EXISTS (
    SELECT 1 FROM `conditions` existing
    WHERE existing.`SourceTypeOrReferenceId` = rules.`source` AND existing.`SourceGroup` = rules.`group_id`
      AND existing.`SourceEntry` = rules.`entry` AND existing.`SourceId` = 0 AND existing.`ElseGroup` = 0
      AND existing.`ConditionTypeOrReference` = rules.`type` AND existing.`ConditionTarget` = 0
      AND existing.`ConditionValue1` = rules.`v1` AND existing.`ConditionValue2` = rules.`v2`
      AND existing.`ConditionValue3` = 0 AND existing.`NegativeCondition` = rules.`negative`
);

-- Native gameobject summons interpret the lifetime in seconds.
UPDATE `smart_scripts` SET `action_param2` = 300
WHERE `entryorguid` = 34356 AND `source_type` = 0 AND `id` = 0 AND `event_type` = 8
  AND `event_param1` = 65300 AND `action_type` = 50 AND `action_param1` = 195055 AND `action_param2` = 300000;

-- Preserve the authored fragment chance and the other possible contents.
UPDATE `gameobject_loot_template` SET `QuestRequired` = 1
WHERE `Entry` = 195055 AND `Item` = 46702;

UPDATE `quest_template`
SET `ItemDrop1` = 46702, `ItemDropQuantity1` = 5, `ItemDrop2` = 46387, `ItemDropQuantity2` = 1
WHERE `ID` = 13918 AND `ItemDrop1` = 0 AND `ItemDrop2` = 0;
