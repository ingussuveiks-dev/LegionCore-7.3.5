-- War Councilor Victoria (117871) starts each Mage Tower encounter by casting
-- the corresponding teleport spell. Reuse the exact target positions already
-- stored for those spells as the LFG scenario entrances.
INSERT IGNORE INTO `lfg_entrances`
    (`dungeonId`, `name`, `position_x`, `position_y`, `position_z`, `orientation`)
VALUES
    (1406, 'The Archmage''s Reckoning', 764.04, 1333.36, 267.23, 0),
    (1417, 'The Imp Mother''s Den', -369.02, 7580.44, 53.19, 1.21981),
    (1440, 'Kruul''s Intentions', 3986.55, -2358.3, 98.75, 4.73578),
    (1445, 'Feltotem''s Fall', 5365.22, 5021.48, 650.62, 3.63482),
    (1446, 'The God-Queen''s Fury', 3472.16, 529.13, 616.5, 3.14229),
    (1452, 'Thwarting the Twins', 3470.62, 1505.72, 436.88, 2.39599),
    (1471, 'The End of the Risen Threat', 3291.3, 7266.57, 231.45, 2.40855);
