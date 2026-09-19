-- Restore the Legion 7.3.5 Arms Warrior proc/filter handlers that cannot be
-- expressed by the client SpellEffect records alone.

DELETE FROM `spell_script_names` WHERE `ScriptName` IN
('spell_warr_tactician', 'spell_warr_executioners_precision',
 'spell_warr_focused_rage_arms', 'spell_warr_overpower_passive',
 'spell_warr_precise_strikes', 'spell_warr_in_for_the_kill',
 'spell_warr_trauma');

INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(184783, 'spell_warr_tactician'),
(238147, 'spell_warr_executioners_precision'),
(207982, 'spell_warr_focused_rage_arms'),
(119938, 'spell_warr_overpower_passive'),
(248579, 'spell_warr_precise_strikes'),
(248621, 'spell_warr_in_for_the_kill'),
(215538, 'spell_warr_trauma');
