-- Demon Hunter (Havoc): restore Legion 7.3.5 soul-fragment talent behavior.
-- Consume Soul always heals, but its 35 Fury trigger requires Demonic Appetite.

DELETE FROM `spell_script_names`
WHERE `spell_id` IN (178963, 203794, 228532, 202644)
  AND `ScriptName` IN ('spell_dh_consume_soul', 'spell_dh_consume_soul_demon');

INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(178963, 'spell_dh_consume_soul'),
(203794, 'spell_dh_consume_soul'),
(228532, 'spell_dh_consume_soul'),
(202644, 'spell_dh_consume_soul_demon');
