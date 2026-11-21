-- Restore the unique shared Thorn Hill quest target from the original template.
-- The horizontal position comes from the reference database; use Legion's terrain height.
INSERT INTO `creature`
    (`id`, `map`, `zoneId`, `areaId`, `spawnMask`, `phaseMask`, `position_x`, `position_y`, `position_z`, `orientation`, `spawntimesecs`)
SELECT 34544, 1, 17, 1699, 1, 1, -37.4816, -3351.61, 91.7306, 0.838339, 120
WHERE EXISTS (SELECT 1 FROM `creature_template` WHERE `entry` = 34544)
  AND NOT EXISTS (SELECT 1 FROM `creature` WHERE `id` = 34544);

-- Reconnect the template to its existing Quilboar Rush on-aggro row.
UPDATE `creature_template` SET `AIName` = 'SmartAI'
WHERE `entry` = 34544 AND `AIName` = '' AND `ScriptName` = ''
  AND EXISTS (SELECT 1 FROM `smart_scripts` WHERE `entryorguid` = 34544 AND `source_type` = 0
      AND `event_type` = 4 AND `action_type` = 11 AND `action_param1` = 82610 AND `target_type` = 2);
