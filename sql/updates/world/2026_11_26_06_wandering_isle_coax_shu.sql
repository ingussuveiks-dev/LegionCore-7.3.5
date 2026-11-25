-- Liang's Sun Pearl must be used at the Pool of Reflection after the crossing.
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`)
SELECT 103116, 'spell_wandering_isle_coax_shu'
WHERE NOT EXISTS (SELECT 1 FROM `spell_script_names` WHERE `spell_id` = 103116);
