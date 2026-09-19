UPDATE `spell_script_names`
SET `ScriptName` = 'spell_nythendra_rot_dot'
WHERE `spell_id` = 203096
  AND `ScriptName` = 'spell_nythendra_volatile_rot';

INSERT IGNORE INTO `spell_script_names` (`spell_id`, `ScriptName`)
VALUES (215300, 'spell_elerethe_web_of_pain_periodic');
