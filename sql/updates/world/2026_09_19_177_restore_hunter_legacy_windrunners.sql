-- Legacy of the Windrunners (190852) uses server-side logic in Legion 7.3.5:
-- its DB2 trigger (190854) is only a hidden bookkeeping aura, while a proc
-- launches six copies of the dedicated Aimed Shot variant (191043).
DELETE FROM `spell_script_names`
WHERE `spell_id` = 190852
  AND `ScriptName` = 'spell_hun_legacy_of_the_windrunners';

INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(190852, 'spell_hun_legacy_of_the_windrunners');
