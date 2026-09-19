-- Touch of Elune (154748) is a passive selector. Its DB2 effect is only a
-- dummy aura; the server must apply the day (154796) or night (154797) stat
-- aura according to realm time.
DELETE FROM `spell_script_names`
WHERE `spell_id` = 154748 AND `ScriptName` = 'spell_gen_touch_of_elune';

INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(154748, 'spell_gen_touch_of_elune');
