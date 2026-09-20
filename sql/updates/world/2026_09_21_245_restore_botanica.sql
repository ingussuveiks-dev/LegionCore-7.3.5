-- Restore The Botanica instance state and replace incomplete boss SmartAI scripts.
UPDATE `instance_template`
SET `script` = 'instance_the_botanica'
WHERE `map` = 553;

UPDATE `creature_template`
SET `AIName` = '', `ScriptName` = 'boss_commander_sarannis'
WHERE `entry` = 17976;

UPDATE `creature_template`
SET `AIName` = '', `ScriptName` = 'boss_thorngrin_the_tender'
WHERE `entry` = 17978;

DELETE FROM `smart_scripts`
WHERE `source_type` = 0 AND `entryorguid` IN (17976, 17978);
