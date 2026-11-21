START TRANSACTION;

UPDATE `creature_template`
SET `flags_extra` = `flags_extra` | 128
WHERE `entry` = 118263 AND `AIName` = '' AND `ScriptName` = '';

INSERT INTO `smart_scripts`
(`entryorguid`, `source_type`, `id`, `event_type`, `event_param1`, `event_param2`, `event_param3`, `event_param4`,
 `action_type`, `action_param1`, `target_type`, `comment`)
VALUES
(117198, 0, 19, 0, 6000, 9000, 20000, 27000, 11, 234920, 1,
 'Highlord Kruul - In Combat - Shadow Sweep warning')
ON DUPLICATE KEY UPDATE `id` = VALUES(`id`);

INSERT INTO `conditions`
(`SourceTypeOrReferenceId`, `SourceGroup`, `SourceEntry`, `SourceId`, `ElseGroup`, `ConditionTypeOrReference`,
 `ConditionTarget`, `ConditionValue1`, `ConditionValue2`, `ConditionValue3`, `NegativeCondition`, `Comment`)
VALUES
(22, 20, 117198, 0, 0, 44, 1, 1259, 2, 0, 0, 'Shadow Sweep requires the final arena step'),
(13, 3, 234922, 0, 0, 31, 0, 3, 118263, 0, 0, 'Shadow Sweep activation searches for Shadow Sweepers')
ON DUPLICATE KEY UPDATE `Comment` = VALUES(`Comment`);

-- The native event 56402 sends the boss's summon spell back to its source.
INSERT INTO `event_scripts` (`id`, `delay`, `command`, `datalong`, `datalong2`, `dataint`)
VALUES (56402, 0, 15, 234789, 1, 1)
ON DUPLICATE KEY UPDATE `id` = VALUES(`id`);

INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`)
VALUES (234789, 'spell_mage_tower_kruul_shadow_sweep'),
       (234920, 'spell_mage_tower_kruul_shadow_sweep'),
       (234922, 'spell_mage_tower_kruul_shadow_sweep')
ON DUPLICATE KEY UPDATE `ScriptName` = VALUES(`ScriptName`);

INSERT INTO `areatrigger_scripts` (`entry`, `ScriptName`)
VALUES (13542, 'at_mage_tower_kruul_shadow_sweep')
ON DUPLICATE KEY UPDATE `ScriptName` = VALUES(`ScriptName`);

COMMIT;
