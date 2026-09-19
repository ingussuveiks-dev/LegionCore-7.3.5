-- Restore Legion 7.3.5 Windwalker mechanics that require server-side spell handling.

DELETE FROM `spell_script_names`
WHERE `spell_id` = 116092
  AND `ScriptName` = 'spell_monk_afterlife';

INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(116092, 'spell_monk_afterlife');
