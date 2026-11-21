START TRANSACTION;

-- Only a friendly player can consume the ward. Keep its single native charge.
UPDATE `areatrigger_actions`
SET `targetFlags` = 9
WHERE `entry` = 9586 AND `customEntry` = 0 AND `id` = 0
  AND `moment` = 1 AND `actionType` = 0 AND `spellId` = 240521
  AND `maxCharges` = 1 AND `targetFlags` = 1;

-- Start the ward timer in combat, and preserve its priority over Smite.
UPDATE `smart_scripts`
SET `event_type` = 0, `event_param1` = 8000, `event_param2` = 16000,
    `event_param3` = 33000, `event_param4` = 33000
WHERE `entryorguid` = 117042 AND `source_type` = 0 AND `id` = 14
  AND `event_type` = 60 AND `action_type` = 11 AND `action_param1` = 233473;

-- Smite waits for the ongoing ward instead of interrupting it.
UPDATE `smart_scripts`
SET `action_param2` = 0
WHERE `entryorguid` = 117042 AND `source_type` = 0 AND `id` = 16
  AND `event_type` = 60 AND `action_type` = 11 AND `action_param1` = 239736 AND `action_param2` = 1;

INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`)
VALUES (240521, 'spell_mage_tower_highlords_return_holy_ward')
ON DUPLICATE KEY UPDATE `ScriptName` = VALUES(`ScriptName`);

COMMIT;
