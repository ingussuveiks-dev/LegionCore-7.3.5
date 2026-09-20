-- Restore Mana-Tombs instance state and replace Tavarok's partial SmartAI.
UPDATE `instance_template`
SET `script` = 'instance_mana_tombs'
WHERE `map` = 557;

UPDATE `creature_template`
SET `AIName` = '', `ScriptName` = 'boss_tavarok'
WHERE `entry` = 18343;

DELETE FROM `smart_scripts`
WHERE `source_type` = 0 AND `entryorguid` = 18343;
