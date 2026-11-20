START TRANSACTION;

-- The actual impact spell creates this native ten-second ground effect.
INSERT INTO `areatrigger_template`
    (`entry`, `spellId`, `customEntry`, `Radius`, `RadiusTarget`, `comment`)
VALUES (9645, 235261, 1009645, 6, 6, 'An Impossible Foe - Fel Lava Blast')
ON DUPLICATE KEY UPDATE `comment` = VALUES(`comment`);

INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`)
VALUES (235261, 'spell_mage_tower_agatha_fel_lava')
ON DUPLICATE KEY UPDATE `ScriptName` = VALUES(`ScriptName`);

COMMIT;
