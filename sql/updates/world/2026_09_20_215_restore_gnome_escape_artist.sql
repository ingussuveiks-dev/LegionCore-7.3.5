-- Escape Artist (20589) is a script effect in 7.3.5 and must remove root and
-- snare mechanics. The legacy self-link only recast the same spell and never
-- implemented its racial behavior.
DELETE FROM `spell_linked_spell`
WHERE `spell_trigger` = 20589 AND `spell_effect` = 20589 AND `type` = 0;

DELETE FROM `spell_script_names`
WHERE `spell_id` = 20589 AND `ScriptName` = 'spell_gen_escape_artist';

INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(20589, 'spell_gen_escape_artist');
