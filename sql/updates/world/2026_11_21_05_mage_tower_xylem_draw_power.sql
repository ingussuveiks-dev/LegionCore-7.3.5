START TRANSACTION;

UPDATE `creature_template`
SET `AIName` = '', `ScriptName` = 'npc_mage_tower_xylem'
WHERE `entry` = 115244 AND `AIName` = 'SmartAI' AND `ScriptName` = '';

INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`)
VALUES (231522, 'spell_mage_tower_xylem_draw_power')
ON DUPLICATE KEY UPDATE `ScriptName` = VALUES(`ScriptName`);

COMMIT;
