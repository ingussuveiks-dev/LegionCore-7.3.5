-- These two entries are exclusive to the Southshore vs. Tarren Mill battlefield.
-- Keep the client creature models and elite health multipliers.
UPDATE `creature_template`
SET `minlevel` = 110, `maxlevel` = 110, `exp` = 6, `HealthScalingExpansion` = 6,
    `faction` = CASE `entry` WHEN 84557 THEN 1216 ELSE 1214 END, `baseattacktime` = 2000,
    `rangeattacktime` = 2000, `AIName` = 'AggressorAI', `flags_extra` = `flags_extra` | 64
WHERE `entry` IN (84557, 84593);

INSERT IGNORE INTO `creature_equip_template` (`CreatureID`, `ID`, `ItemID1`, `ItemID2`, `ItemID3`, `ItemID4`, `ItemID5`, `ItemID6`)
SELECT CASE `CreatureID` WHEN 2386 THEN 84557 ELSE 84593 END, `ID`, `ItemID1`, `ItemID2`, `ItemID3`, `ItemID4`, `ItemID5`, `ItemID6`
FROM `creature_equip_template` WHERE `CreatureID` IN (2386, 2405) AND `ID` = 1;

-- Reconstructed walk between the farm corner and the outside of the orchard fence.
INSERT INTO `waypoint_data` (`id`, `point`, `position_x`, `position_y`, `position_z`, `delay`, `delay_chance`, `move_type`)
VALUES (845930, 1, 2595.50, 703.75, 55.4762, 2000, 100, 0),
       (845930, 2, 2605.25, 699.00, 55.4913, 2000, 100, 0)
ON DUPLICATE KEY UPDATE `position_x` = VALUES(`position_x`), `position_y` = VALUES(`position_y`),
                        `position_z` = VALUES(`position_z`), `delay` = VALUES(`delay`),
                        `delay_chance` = VALUES(`delay_chance`), `move_type` = VALUES(`move_type`);
