START TRANSACTION;

UPDATE `creature_template`
SET `AIName` = '', `ScriptName` = 'npc_mage_tower_risen_threat_soul'
WHERE `entry` IN (119470, 119476, 119477, 119478, 119479, 119480)
  AND `AIName` = 'SmartAI' AND `ScriptName` = '';

UPDATE `smart_scripts`
SET `event_param1` = 100
WHERE `source_type` = 0 AND `id` = 3 AND `event_type` = 2
  AND `event_param1` = 95 AND `event_param2` = 100
  AND ((`entryorguid` = 119470 AND `action_type` = 1)
    OR (`entryorguid` IN (119476, 119477, 119478, 119479, 119480)
      AND `action_type` = 205 AND `action_param1` = 92 AND `action_param2` = 56911));

-- Keep the original in-flight aura and phase gates on the five wave types.
UPDATE `conditions`
SET `ScriptName` = 'condition_mage_tower_risen_threat_full_heal'
WHERE `SourceTypeOrReferenceId` = 22 AND `SourceGroup` = 4 AND `SourceId` = 0 AND `ElseGroup` = 0
  AND `SourceEntry` IN (119476, 119477, 119478, 119479, 119480)
  AND `ConditionTypeOrReference` = 44 AND `ConditionTarget` = 1
  AND `ConditionValue1` = 1281 AND `ConditionValue2` = 4 AND `NegativeCondition` = 0 AND `ScriptName` = '';

INSERT INTO `conditions`
(`SourceTypeOrReferenceId`, `SourceGroup`, `SourceEntry`, `SourceId`, `ElseGroup`, `ConditionTypeOrReference`,
 `ConditionTarget`, `ConditionValue1`, `ConditionValue2`, `ConditionValue3`, `NegativeCondition`, `ScriptName`, `Comment`)
VALUES
(22, 4, 119470, 0, 0, 44, 1, 1281, 2, 0, 0, 'condition_mage_tower_risen_threat_full_heal',
 'Risen Threat Damaged Soul needs a real full heal during the rescue')
ON DUPLICATE KEY UPDATE `Comment` = VALUES(`Comment`);

COMMIT;
