-- The spell-click NPC "Rift to Telogrus" (130877) is outside the scenario on
-- map 530 and casts spell 250798. Use that spell's existing map 1622 target;
-- the other Telogrus target spells belong to internal Void Rift portals.
INSERT IGNORE INTO `lfg_entrances`
    (`dungeonId`, `name`, `position_x`, `position_y`, `position_z`, `orientation`)
VALUES
    (1616, 'Telogrus Rift', 1385.32, 2859.3, 58.1648, 1.77329);
