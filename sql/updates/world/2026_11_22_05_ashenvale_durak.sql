-- Keep the original gossip, speech, spawn and two sequential objectives.
-- Durak's controller waits for the native transform before opening combat.
UPDATE `creature_template`
SET `AIName` = '', `ScriptName` = 'npc_ashenvale_durak'
WHERE `entry` = 44414 AND `AIName` = 'SmartAI' AND `ScriptName` = '';

DELETE FROM `smart_scripts`
WHERE `entryorguid` = 44414 AND `source_type` = 0 AND `id` IN (0,1,2,3,4,5,6) AND
    EXISTS (SELECT 1 FROM `creature_template` WHERE `entry` = 44414 AND `ScriptName` = 'npc_ashenvale_durak');
