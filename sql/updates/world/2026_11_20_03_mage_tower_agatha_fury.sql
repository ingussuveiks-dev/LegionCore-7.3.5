-- The native 242879/242880 sequence belongs to actual imp deaths.
START TRANSACTION;

UPDATE `creature_template`
SET `AIName` = '', `ScriptName` = 'npc_mage_tower_agatha'
WHERE `entry` = 115638 AND `AIName` = 'SmartAI' AND `ScriptName` = '';

DELETE script
FROM `smart_scripts` AS script
JOIN `creature_template` AS agatha ON agatha.`entry` = script.`entryorguid`
WHERE agatha.`entry` = 115638 AND agatha.`ScriptName` = 'npc_mage_tower_agatha'
  AND script.`source_type` = 0 AND script.`id` = 14 AND script.`link` = 0
  AND script.`event_type` = 0 AND script.`action_type` = 11
  AND script.`action_param1` = 242879 AND script.`target_type` = 2;

COMMIT;
