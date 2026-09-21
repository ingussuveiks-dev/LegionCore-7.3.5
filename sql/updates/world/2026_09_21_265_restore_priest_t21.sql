-- Shadow T21 4P is applied by the existing Voidform aura script as Voidform
-- stacks change. The remaining Priest T21 bonuses use native aura effects.
DELETE FROM `spell_script_names`
WHERE `spell_id` = 194249 AND `ScriptName` = 'spell_pri_voidform';

INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(194249, 'spell_pri_voidform');
