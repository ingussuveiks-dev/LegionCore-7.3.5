START TRANSACTION;

-- Bind the original teleport spells. Their SmartAI selection, specialization and scenario rows stay intact.
INSERT INTO spell_script_names (spell_id, ScriptName) VALUES
(229196, 'spell_mage_tower_victoria_admission'),
(231857, 'spell_mage_tower_victoria_admission'),
(233379, 'spell_mage_tower_victoria_admission'),
(233380, 'spell_mage_tower_victoria_admission'),
(233804, 'spell_mage_tower_victoria_admission'),
(233808, 'spell_mage_tower_victoria_admission'),
(235194, 'spell_mage_tower_victoria_admission'),
(235527, 'spell_mage_tower_victoria_admission'),
(235557, 'spell_mage_tower_victoria_admission'),
(237622, 'spell_mage_tower_victoria_admission'),
(237660, 'spell_mage_tower_victoria_admission'),
(237663, 'spell_mage_tower_victoria_admission'),
(237664, 'spell_mage_tower_victoria_admission')
ON DUPLICATE KEY UPDATE ScriptName = VALUES(ScriptName);

COMMIT;
