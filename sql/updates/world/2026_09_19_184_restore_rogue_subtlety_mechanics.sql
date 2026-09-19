-- Restore Rogue Subtlety mechanics to their 7.3.5 behavior.
DELETE FROM `spell_script_names`
WHERE (`spell_id` = 185314 AND `ScriptName` = 'spell_rog_deepening_shadows')
   OR (`spell_id` = 193537 AND `ScriptName` = 'spell_rog_weaponmaster')
   OR (`spell_id` = 196819 AND `ScriptName` = 'spell_rog_eviscerate')
   OR (`spell_id` = 196912 AND `ScriptName` = 'spell_rog_shadow_techniques')
   OR (`spell_id` = 209781 AND `ScriptName` = 'spell_rog_shadow_nova');

INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(185314, 'spell_rog_deepening_shadows'),
(193537, 'spell_rog_weaponmaster'),
(196819, 'spell_rog_eviscerate'),
(196912, 'spell_rog_shadow_techniques'),
(209781, 'spell_rog_shadow_nova');

-- The old Finality links removed and immediately reapplied the buff on every
-- finisher. The spell scripts now alternate consumption and creation, retain
-- the previous Nightblade snapshot, and preserve the combo points of the cast.
DELETE FROM `spell_linked_spell`
WHERE (`spell_trigger` = 195452 AND ABS(`spell_effect`) = 197498)
   OR (`spell_trigger` = 196819 AND ABS(`spell_effect`) = 197496);

-- Shadow Techniques is a fourth/fifth landed-auto-attack sequence. Fortune's
-- Bite modifies that successful proc; it is not a separate random combo point.
DELETE FROM `spell_trigger`
WHERE (`spell_id` = 196912 AND `spell_trigger` = 196911)
   OR (`spell_id` = 197369 AND `spell_trigger` = 196911);

DELETE FROM `spell_proc_event` WHERE `entry` IN (196912, 197369);
