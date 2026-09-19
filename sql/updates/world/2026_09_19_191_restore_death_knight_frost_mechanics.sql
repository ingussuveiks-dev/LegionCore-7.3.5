-- Restore Frost Death Knight 7.3.5 proc filters and Killing Machine consumers.
DELETE FROM `spell_script_names`
WHERE `ScriptName` IN
(
    'spell_dk_killing_machine_consumer',
    'spell_dk_icecap',
    'spell_dk_avalanche',
    'spell_dk_abominations_might'
);

INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(49020,  'spell_dk_killing_machine_consumer'),
(207230, 'spell_dk_killing_machine_consumer'),
(207126, 'spell_dk_icecap'),
(207142, 'spell_dk_avalanche'),
(207161, 'spell_dk_abominations_might');

-- Killing Machine must be consumed only after Obliterate/Frostscythe lands;
-- the SpellScript also grants Murderous Efficiency's rune exactly once.
DELETE FROM `spell_linked_spell`
WHERE `spell_trigger` = 207230 AND `spell_effect` = -51124 AND `type` = 0;

-- Permafrost absorbs damage on the Death Knight, not on the attacked unit.
UPDATE `spell_trigger`
SET `target` = 1
WHERE `spell_id` = 207200 AND `spell_trigger` = 207203 AND `option` = 3;
