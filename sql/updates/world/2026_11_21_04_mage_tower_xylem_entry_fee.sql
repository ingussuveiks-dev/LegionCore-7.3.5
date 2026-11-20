START TRANSACTION;

UPDATE `creature_template`
SET `AIName` = '', `ScriptName` = 'npc_mage_tower_victoria'
WHERE `entry` = 117871 AND `AIName` = 'SmartAI' AND `ScriptName` = '';

COMMIT;
