-- Restore the Legion 7.3.5 server-side parts of Templar's Verdict and Divine Hammer.
DELETE FROM `spell_script_names`
WHERE `spell_id` IN (85256, 198034)
  AND `ScriptName` IN ('spell_pal_templars_verdict', 'spell_pal_divine_hammer');

INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(85256,  'spell_pal_templars_verdict'),
(198034, 'spell_pal_divine_hammer');
