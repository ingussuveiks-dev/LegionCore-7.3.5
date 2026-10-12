-- The paid UA cast must check all five of its own DoTs on the enemy.
-- The old link redirected its target to the caster before checking the aura.
DELETE FROM `spell_linked_spell`
WHERE `spell_trigger` = 233490 AND `spell_effect` = 205756;

DELETE FROM `spell_script_names`
WHERE `spell_id` = 30108 AND `ScriptName` = 'spell_warl_power_cord_of_lethtendris';
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`)
VALUES (30108, 'spell_warl_power_cord_of_lethtendris');
