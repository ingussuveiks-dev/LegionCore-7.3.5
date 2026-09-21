-- Halion now summons GO 202796 directly during the phase transition. The
-- placeholder spell 123456 is not present in the 7.3.5 client and its named
-- script has no implementation in the core.
DELETE FROM `spell_script_names`
WHERE `spell_id` = 123456
  AND `ScriptName` = 'spell_halion_summon_exit_portals';
