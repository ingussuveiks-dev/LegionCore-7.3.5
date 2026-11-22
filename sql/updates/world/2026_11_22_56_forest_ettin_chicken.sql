START TRANSACTION;

-- Matching client vehicle 1063 has the original arm and mouth seats.
UPDATE `creature_template`
SET `VehicleId` = 1063, `ScriptName` = 'npc_silverpine_forest_ettin'
WHERE `entry` = 44367 AND `VehicleId` IN (0, 1063)
  AND `ScriptName` IN ('', 'npc_silverpine_forest_ettin');

INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`)
VALUES
(83902, 'spell_silverpine_release_bush_chicken'),
(83904, 'spell_silverpine_ettin_grab'),
(83903, 'spell_silverpine_chicken_explosion')
ON DUPLICATE KEY UPDATE `ScriptName` = VALUES(`ScriptName`);

COMMIT;
