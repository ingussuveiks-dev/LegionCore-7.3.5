START TRANSACTION;

UPDATE `creature_template`
SET `AIName` = '', `ScriptName` = 'npc_mage_tower_corrupting_shadows'
WHERE `entry` = 116839 AND `AIName` = 'SmartAI' AND `ScriptName` = '';

INSERT INTO `areatrigger_template`
    (`entry`, `spellId`, `customEntry`, `Radius`, `RadiusTarget`, `HasDynamicShape`, `comment`)
VALUES (0, 232664, 90232664, 5, 100, 1, 'Closing the Eye - Creeping Shadows')
ON DUPLICATE KEY UPDATE `comment` = VALUES(`comment`);

COMMIT;
