-- Legion 7.3.5 Warlock (Affliction): restore server-side target selection
-- for Sow the Seeds and the periodic damage driver for Phantom Singularity.

DELETE FROM `spell_script_names`
WHERE (`spell_id` = 27243  AND `ScriptName` = 'spell_warl_sow_the_seeds')
   OR (`spell_id` = 205179 AND `ScriptName` = 'spell_warl_phantom_singularity');

INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(27243,  'spell_warl_sow_the_seeds'),
(205179, 'spell_warl_phantom_singularity');
