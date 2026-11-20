START TRANSACTION;

-- These are grounded positions at the ends of Agatha's phase-6318 arena.
-- The two original effect-252 spells read their destinations through the
-- native nearby-entry fallback in Spell::SelectImplicitNearbyTargets.
INSERT INTO `spell_target_position`
    (`id`, `target_map`, `target_position_x`, `target_position_y`, `target_position_z`, `target_orientation`)
VALUES
    (242987, 1616, -331, 7688, 63.55, 4.364),
    (242989, 1616, -367, 7589, 53.247, 1.222)
ON DUPLICATE KEY UPDATE `id` = VALUES(`id`);

COMMIT;
