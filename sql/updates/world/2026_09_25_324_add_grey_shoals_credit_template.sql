-- 115739 is a virtual quest kill credit, not a world creature spawn.
-- Keep a server creature template for the objective without placing it in the world.
INSERT INTO `creature_template`
    (`entry`, `minlevel`, `maxlevel`, `faction`, `WorldEffects`, `PassiveSpells`)
VALUES (115739, 1, 1, 35, '', '')
ON DUPLICATE KEY UPDATE `entry` = VALUES(`entry`);
