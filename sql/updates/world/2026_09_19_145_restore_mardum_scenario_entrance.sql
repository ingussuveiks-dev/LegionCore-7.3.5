-- Spell 232169 is the only stored teleport target for map 1523. Its target is
-- the Mardum scenario's Illidari staging area beside the first DH CH Quest
-- event object, so use that existing position for the LFG entrance.
INSERT IGNORE INTO `lfg_entrances`
    (`dungeonId`, `name`, `position_x`, `position_y`, `position_z`, `orientation`)
VALUES
    (1094, 'Mardum Scenario', 1470.44, 1411.41, 243.73, 0.0300197);
