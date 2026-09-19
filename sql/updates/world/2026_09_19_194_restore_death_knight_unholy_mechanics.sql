-- Restore missing 7.3.5 Unholy Death Knight spell behavior.
DELETE FROM `spell_script_names`
WHERE `ScriptName` IN
(
    'spell_dk_epidemic',
    'spell_dk_epidemic_aoe',
    'spell_dk_death_coil',
    'spell_dk_virulent_plague',
    'spell_dk_virulent_eruption',
    'spell_dk_shadow_infusion'
);

INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(47541,  'spell_dk_death_coil'),
(207317, 'spell_dk_epidemic'),
(215969, 'spell_dk_epidemic_aoe'),
(191587, 'spell_dk_virulent_plague'),
(191685, 'spell_dk_virulent_eruption'),
(198943, 'spell_dk_shadow_infusion');

-- Shadow Infusion is handled in C++ so it can require Death Coil and reject
-- procs while the ghoul/abomination is Dark Transformed.
DELETE FROM `spell_trigger`
WHERE `spell_id` = 198943 AND `spell_trigger` = 63560 AND `option` = 23;

-- Death and Decay and Defile use hostile target lists. In this core their
-- owner-targeted generic actions are consequently validated against enemies
-- and never reach the caster. AreaTriggerAI owns the caster's cleave and
-- Lanathel's Lament buffs, including correct behavior for overlapping grounds.
DELETE FROM `areatrigger_scripts` WHERE `entry` IN (4485, 1713);
INSERT INTO `areatrigger_scripts` (`entry`, `ScriptName`) VALUES
(4485, 'at_dk_death_and_decay');
INSERT INTO `areatrigger_scripts` (`entry`, `ScriptName`) VALUES
(1713, 'at_dk_defile');

DELETE FROM `areatrigger_actions`
WHERE (`entry` = 4485 AND `customEntry` = 9225 AND `id` IN (1, 2, 3, 4))
   OR (`entry` = 1713 AND `customEntry` = 6212 AND `id` IN (3, 4, 5, 6, 7, 8));
