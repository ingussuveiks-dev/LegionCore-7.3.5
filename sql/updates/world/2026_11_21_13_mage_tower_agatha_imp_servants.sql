START TRANSACTION;

UPDATE `creature_template`
SET `AIName` = '', `ScriptName` = 'npc_mage_tower_agatha_imp_servant'
WHERE `entry` IN (115643, 115719) AND `AIName` = 'SmartAI' AND `ScriptName` = '';

INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`)
VALUES (229928, 'spell_mage_tower_agatha_funnel_energy')
ON DUPLICATE KEY UPDATE `ScriptName` = VALUES(`ScriptName`);

COMMIT;
