-- Restore missing Legion 7.3.5 Marksmanship talent and Thas'dorah mechanics.
DELETE FROM `spell_script_names`
WHERE `spell_id` IN (190503, 191048, 191328, 191339, 194595, 199527, 204089, 238088)
  AND `ScriptName` IN
  (
      'spell_hun_healing_shell',
      'spell_hun_call_of_the_hunter',
      'spell_hun_critical_focus',
      'spell_hun_rapid_killing',
      'spell_hun_lock_and_load',
      'spell_hun_true_aim',
      'spell_hun_bullseye',
      'spell_hun_feet_of_wind'
  );

INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(190503, 'spell_hun_healing_shell'),
(191048, 'spell_hun_call_of_the_hunter'),
(191328, 'spell_hun_critical_focus'),
(191339, 'spell_hun_rapid_killing'),
(194595, 'spell_hun_lock_and_load'),
(199527, 'spell_hun_true_aim'),
(204089, 'spell_hun_bullseye'),
(238088, 'spell_hun_feet_of_wind');

-- Sentinel creates custom AreaTrigger entry 14691 (template entry 9769).
DELETE FROM `areatrigger_scripts`
WHERE `entry` = 14691
  AND `ScriptName` = 'areatrigger_hun_sentinel';

INSERT INTO `areatrigger_scripts` (`entry`, `ScriptName`) VALUES
(14691, 'areatrigger_hun_sentinel');
