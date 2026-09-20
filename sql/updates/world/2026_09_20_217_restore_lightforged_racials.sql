-- Light's Judgment has no client-side damage coefficient in 7.3.5. Bind its
-- server-side Lightforged Draenei attack-power scaling implementation.
DELETE FROM `spell_script_names`
WHERE `spell_id` = 256893 AND `ScriptName` = 'spell_light_judgement';

INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(256893, 'spell_light_judgement');
