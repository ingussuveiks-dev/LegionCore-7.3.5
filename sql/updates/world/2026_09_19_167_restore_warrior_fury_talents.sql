-- Restore Fury talent behavior which the final client data cannot complete on
-- its own. Fresh Meat and Focus in Chaos are handled in the core calculations.

DELETE FROM `spell_script_names` WHERE `ScriptName` = 'spell_warr_war_machine';

INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(215556, 'spell_warr_war_machine');
