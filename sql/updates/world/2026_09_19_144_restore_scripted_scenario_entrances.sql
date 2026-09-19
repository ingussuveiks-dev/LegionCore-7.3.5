-- These positions are the existing targets of entry spells cast by objects
-- outside the destination scenarios: Spiritwalker Ebonhorn for War of the
-- Ancients and the quest 46812 event object in Northrend for Ruby Sanctum.
INSERT IGNORE INTO `lfg_entrances`
    (`dungeonId`, `name`, `position_x`, `position_y`, `position_z`, `orientation`)
VALUES
    (1429, 'War of the Ancients', 4026.05, -5444.71, 115.8, 1.4844),
    (1502, 'Ruby Sanctum', 3271.46, 533.47, 87.66, 3.12554);
