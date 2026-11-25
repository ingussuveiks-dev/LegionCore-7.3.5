-- Let the native 7.3.5 approach trigger cast the personal Min Dimwind outro.
INSERT INTO `areatrigger_scripts` (`entry`, `ScriptName`)
VALUES (6958, 'at_min_dimwind_captured')
ON DUPLICATE KEY UPDATE `ScriptName` = VALUES(`ScriptName`);
