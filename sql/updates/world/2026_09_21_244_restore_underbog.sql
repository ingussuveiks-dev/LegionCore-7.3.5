-- Restore The Underbog instance state and replace incomplete boss SmartAI scripts.
UPDATE `instance_template`
SET `script` = 'instance_the_underbog'
WHERE `map` = 546;

UPDATE `creature_template`
SET `AIName` = '', `ScriptName` = 'boss_ghazan'
WHERE `entry` = 18105;

UPDATE `creature_template`
SET `AIName` = '', `ScriptName` = 'boss_swamplord_muselek'
WHERE `entry` = 17826;

UPDATE `creature_template`
SET `AIName` = '', `ScriptName` = 'npc_underbog_claw'
WHERE `entry` IN (17827, 17894);

DELETE FROM `smart_scripts`
WHERE (`source_type` = 0 AND `entryorguid` IN (17826, 17827, 18105))
   OR (`source_type` = 9 AND `entryorguid` IN (1782600, 1782700, 1810500));
