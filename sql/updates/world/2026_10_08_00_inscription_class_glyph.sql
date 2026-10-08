-- 7.3.5.26972: reconstruct the missing server-side class recipe dispatch.
-- Keep quest 39931 and the profession relearning spell using the same reward.
DELETE FROM `spell_script_names` WHERE `spell_id` = 192962 AND `ScriptName` = 'spell_gen_inscription_class_glyph';
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(192962, 'spell_gen_inscription_class_glyph');
