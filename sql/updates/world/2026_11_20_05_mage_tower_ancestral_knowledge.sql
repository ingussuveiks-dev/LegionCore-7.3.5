-- Faljar retains his other SmartAI events. The native shield starts its own blast.
START TRANSACTION;

UPDATE `creature_template`
SET `AIName` = '', `ScriptName` = 'npc_mage_tower_faljar'
WHERE `entry` = 116496 AND `AIName` = 'SmartAI' AND `ScriptName` = '';

INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`)
VALUES (237952, 'spell_mage_tower_ancestral_blast')
ON DUPLICATE KEY UPDATE `ScriptName` = VALUES(`ScriptName`);

COMMIT;
