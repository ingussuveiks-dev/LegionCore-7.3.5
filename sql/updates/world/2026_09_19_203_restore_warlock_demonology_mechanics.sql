-- Restore Legion 7.3.5 Demonology mechanics that require server-side logic.
DELETE FROM `spell_script_names`
WHERE `spell_id` IN (603, 196277)
  AND `ScriptName` IN ('spell_warl_doom', 'spell_warl_implosion');

INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(603,    'spell_warl_doom'),
(196277, 'spell_warl_implosion');

-- Improved Dreadstalkers uses a second Wild Imp creature entry. Give it the
-- same lifecycle/combat script as Hand of Gul'dan's Wild Imps so artifact
-- traits (including The Expendables) work for both summon sources.
UPDATE `creature_template`
SET `ScriptName` = 'npc_wild_imp'
WHERE `entry` = 99737;
