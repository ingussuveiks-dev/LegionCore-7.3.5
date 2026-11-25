-- Use the native crossing spell only after a completed Reflection landing.
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`)
SELECT 108590, 'spell_wandering_isle_reflection_crossing'
WHERE NOT EXISTS (SELECT 1 FROM `spell_script_names` WHERE `spell_id` = 108590);
