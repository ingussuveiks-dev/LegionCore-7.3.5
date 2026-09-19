-- External DK class-hall quest event objects 184 and 190 already teleport
-- players to these exact scenario start positions. Reuse their SmartAI
-- targets for the matching LFG dungeon records.
INSERT IGNORE INTO `lfg_entrances`
    (`dungeonId`, `name`, `position_x`, `position_y`, `position_z`, `orientation`)
VALUES
    (1295, 'Rescue Koltira', 1675.92, 727.864, 77.6129, 2.893),
    (1317, 'The Fourth Horseman', 2407.95, -5160.34, 82.1714, 4.351);
