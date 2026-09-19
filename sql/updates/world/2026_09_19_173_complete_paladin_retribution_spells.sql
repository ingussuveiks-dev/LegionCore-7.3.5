-- Complete the Legion 7.3.5 Word of Glory and Healing Storm server logic.
DELETE FROM `spell_script_names`
WHERE `spell_id` IN (210191, 215257)
  AND `ScriptName` IN ('spell_pal_word_of_glory', 'spell_pal_healing_storm');

INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(210191, 'spell_pal_word_of_glory'),
(215257, 'spell_pal_healing_storm');
