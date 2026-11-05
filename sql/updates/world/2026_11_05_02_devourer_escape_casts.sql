-- Native cage release and weapon reclaim admission; code-coupled, keep outside
-- updater input until the matching Release executable is ready to promote.
DELETE FROM `spell_script_names` WHERE `spell_id` IN (211687,211689) AND `ScriptName`='spell_devourer_escape';
INSERT INTO `spell_script_names` (`spell_id`,`ScriptName`) VALUES
(211687,'spell_devourer_escape'),(211689,'spell_devourer_escape');
