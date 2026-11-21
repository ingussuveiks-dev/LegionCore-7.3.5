START TRANSACTION;

INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`)
VALUES (234676, 'spell_mage_tower_kruul_twisted_reflection')
ON DUPLICATE KEY UPDATE `ScriptName` = VALUES(`ScriptName`);

INSERT INTO `smart_scripts`
(`entryorguid`, `source_type`, `id`, `event_type`, `event_param1`, `event_param2`, `event_param3`, `event_param4`,
 `action_type`, `action_param1`, `target_type`, `target_param1`, `comment`)
VALUES
(117198, 0, 18, 0, 18000, 18000, 45000, 45000, 11, 234676, 21, 100,
 'Highlord Kruul - In Combat - Cast Twisted Reflection on the challenger')
ON DUPLICATE KEY UPDATE `id` = VALUES(`id`);

INSERT INTO `conditions`
(`SourceTypeOrReferenceId`, `SourceGroup`, `SourceEntry`, `SourceId`, `ElseGroup`, `ConditionTypeOrReference`,
 `ConditionTarget`, `ConditionValue1`, `ConditionValue2`, `ConditionValue3`, `NegativeCondition`, `Comment`)
VALUES
(22, 19, 117198, 0, 0, 44, 0, 1259, 2, 0, 0, 'Highlord Kruul - Twisted Reflection requires the final arena step')
ON DUPLICATE KEY UPDATE `Comment` = VALUES(`Comment`);

COMMIT;
