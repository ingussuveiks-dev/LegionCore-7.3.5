-- Keep Fluffy, Go synchronized on an active pet when its artifact rank changes.
DELETE FROM `spell_script_names`
WHERE `spell_id` = 203669 AND `ScriptName` = 'spell_hun_fluffy_go';

INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(203669, 'spell_hun_fluffy_go');
