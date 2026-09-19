-- Restore Legion 7.3.5 Brewmaster mechanics that require server-side spell handling.

DELETE FROM `spell_script_names`
WHERE `spell_id` = 195630
  AND `ScriptName` = 'spell_monk_elusive_brawler';

INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(195630, 'spell_monk_elusive_brawler');
