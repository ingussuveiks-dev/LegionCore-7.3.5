-- Restore the Legion 7.3.5 Marksmanship proc restrictions whose DB2 masks
-- are intentionally broad and require spell-specific server validation.
DELETE FROM `spell_script_names`
WHERE `spell_id` IN (35110, 223138)
  AND `ScriptName` IN ('spell_hun_bombardment', 'spell_hun_marking_targets');

INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(35110,  'spell_hun_bombardment'),
(223138, 'spell_hun_marking_targets');
