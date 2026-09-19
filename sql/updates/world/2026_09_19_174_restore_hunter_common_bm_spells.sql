-- Restore missing Legion 7.3.5 Hunter common and Beast Mastery mechanics.
DELETE FROM `spell_script_names`
WHERE `spell_id` IN (781, 53209, 190925, 199523, 199532)
  AND `ScriptName` IN
  (
      'spell_hun_chimaera_shot',
      'spell_hun_chimera_shot',
      'spell_hun_farstrider',
      'spell_hun_killer_cobra',
      'spell_hun_posthaste'
  );

INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(781,    'spell_hun_posthaste'),
(53209,  'spell_hun_chimaera_shot'),
(190925, 'spell_hun_posthaste'),
(199523, 'spell_hun_farstrider'),
(199532, 'spell_hun_killer_cobra');

-- Chimaera Shot's two elemental heads and their Focus gains are selected by
-- the C++ script. The old unconditional Frost-only link made every hit wrong.
DELETE FROM `spell_linked_spell`
WHERE `spell_trigger` = 53209
  AND `spell_effect` = 171454
  AND `type` = 1;
