-- Restore missing Outlaw spell mechanics and replace combo-point-unsafe cooldown triggers.
DELETE FROM `spell_script_names`
WHERE (`spell_id` = 185767 AND `ScriptName` = 'spell_rog_cannonball_barrage')
   OR (`spell_id` = 5171 AND `ScriptName` = 'spell_rog_slice_and_dice')
   OR (`spell_id` IN (79096, 193359) AND `ScriptName` = 'spell_rog_restless_blades');

INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(185767, 'spell_rog_cannonball_barrage'),
(5171, 'spell_rog_slice_and_dice'),
(79096, 'spell_rog_restless_blades'),
(193359, 'spell_rog_restless_blades');

-- These generic option-23 rows read the player's current combo points after the
-- finisher has spent them. The C++ aura scripts use the cast's saved cost instead.
DELETE FROM `spell_trigger` WHERE `spell_id` IN (79096, 193359);
