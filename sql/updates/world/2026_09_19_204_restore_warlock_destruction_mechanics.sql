-- Restore Legion 7.3.5 Destruction mechanics that require server-side logic.
DELETE FROM `spell_script_names`
WHERE (`spell_id` = 193541 AND `ScriptName` = 'spell_warl_immolate_proc')
   OR (`spell_id` = 17962  AND `ScriptName` = 'spell_warl_conflagrate')
   OR (`spell_id` IN (17962, 17877) AND `ScriptName` = 'spell_warl_conflagration_of_chaos')
   OR (`spell_id` = 219195 AND `ScriptName` = 'spell_warl_conflagration_proc')
   OR (`spell_id` = 196412 AND `ScriptName` = 'spell_warl_eradication')
   OR (`spell_id` = 116858 AND `ScriptName` = 'spell_warl_chaos_bolt')
   OR (`spell_id` = 196447 AND `ScriptName` = 'spell_warl_channel_demonfire');

INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(193541, 'spell_warl_immolate_proc'),
(17962,  'spell_warl_conflagrate'),
(17962,  'spell_warl_conflagration_of_chaos'),
(17877,  'spell_warl_conflagration_of_chaos'),
(219195, 'spell_warl_conflagration_proc'),
(196412, 'spell_warl_eradication'),
(116858, 'spell_warl_chaos_bolt'),
(196447, 'spell_warl_channel_demonfire');

-- Dimensional Rift summons are guardians and therefore do not autocast their
-- creature-template spells. Their shared AI restores all four 7.3.5 rifts.
UPDATE `creature_template`
SET `ScriptName` = 'npc_warl_dimensional_rift'
WHERE `entry` IN (94584, 99887, 108493, 121643);

-- Restore the retail creature names used by the corresponding summon spells.
UPDATE `creature_template_wdb` SET `Name1` = 'Unstable Tear' WHERE `Entry` = 94584;
UPDATE `creature_template_wdb` SET `Name1` = 'Shadowy Tear'  WHERE `Entry` = 99887;
UPDATE `creature_template_wdb` SET `Name1` = 'Chaos Tear'    WHERE `Entry` = 108493;
UPDATE `creature_template_wdb` SET `Name1` = 'Flame Rift'    WHERE `Entry` = 121643;
