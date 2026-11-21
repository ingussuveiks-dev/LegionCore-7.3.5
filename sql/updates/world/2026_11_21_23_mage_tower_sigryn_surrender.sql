START TRANSACTION;

-- Keep the native three health-percent events and surrender chain.
INSERT INTO instance_template (map, parent, script, allowMount, bonusChance)
VALUES (1703, 0, 'instance_mage_tower_god_queen', 0, 0)
ON DUPLICATE KEY UPDATE map = VALUES(map);

COMMIT;
