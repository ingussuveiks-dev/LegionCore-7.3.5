START TRANSACTION;

-- Both creature entries and their invisible base display exist in the 26972 WDB data.
INSERT INTO `creature_template`
    (`entry`, `minlevel`, `maxlevel`, `faction`, `unit_flags`, `flags_extra`, `WorldEffects`, `PassiveSpells`, `ScriptName`)
VALUES
    (121109, 110, 110, 14, 33554434, 64, '', '', 'npc_mage_tower_agatha_boulder'),
    (121627, 110, 110, 35, 33554434, 64, '', '', '')
ON DUPLICATE KEY UPDATE `ScriptName` = VALUES(`ScriptName`);

INSERT INTO `areatrigger_template`
    (`entry`, `spellId`, `customEntry`, `Radius`, `RadiusTarget`, `HasAttached`, `comment`)
VALUES (10326, 243570, 1010326, 4.5, 4.5, 1, 'An Impossible Foe - Felfire Boulder attachment')
ON DUPLICATE KEY UPDATE `comment` = VALUES(`comment`);

INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
    (241475, 'spell_mage_tower_agatha_boulder_landing'),
    (241502, 'spell_mage_tower_agatha_boulder_windup')
ON DUPLICATE KEY UPDATE `ScriptName` = VALUES(`ScriptName`);

COMMIT;
