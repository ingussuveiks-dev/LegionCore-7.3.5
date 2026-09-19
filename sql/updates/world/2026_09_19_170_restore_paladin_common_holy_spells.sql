-- Restore server-side conditions encoded by the final 7.3.5 Judgment of Light
-- talent. The debuff application and heal remain driven by client and world DB
-- proc data.

DELETE FROM `spell_script_names`
WHERE `ScriptName` IN
(
    'spell_pal_judgment_of_light',
    'spell_pal_judgment_of_light_debuff'
);

INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(183778, 'spell_pal_judgment_of_light'),
(196941, 'spell_pal_judgment_of_light_debuff');
