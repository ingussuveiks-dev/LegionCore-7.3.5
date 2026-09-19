-- Complete 7.3.5 Shadow Priest procs that need server-side state changes.
DELETE FROM `spell_script_names`
WHERE `ScriptName` IN
(
    'spell_pri_shadowy_insight',
    'spell_pri_void_tendril_mind_flay'
);

INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(162452, 'spell_pri_shadowy_insight'),
(193473, 'spell_pri_void_tendril_mind_flay');
