-- An Ocean Not So Deep (13560): retain the original goober and vehicle ability.
-- Reconstruct 64923's missing DB destination on the seabed beside native spawn 121889.
INSERT IGNORE INTO `spell_target_position`
    (`id`, `target_map`, `target_position_x`, `target_position_y`, `target_position_z`, `target_orientation`)
VALUES (64923, 1, 7780.38, -422.381, -22.0, 0);

-- A canceled aura must not blow up the still-present vehicle. Keep the native eject link.
UPDATE `spell_linked_spell` SET `removeMask` = 16
WHERE `spell_trigger` = -62684 AND `spell_effect` = 62682 AND `type` = 0 AND `removeMask` = 0;

-- The two old SPELLHIT actions grant credit before distinguishing lethal damage.
-- The scoped 62682 handler now grants native group event credit on a real death.
DELETE FROM `smart_scripts`
WHERE `entryorguid` IN (33262, 33277) AND `source_type` = 0 AND `id` = 0 AND `link` = 1
  AND `event_type` = 8 AND `event_param1` = 62682
  AND `action_type` = 85 AND `action_param1` = 62683 AND `target_type` = 7;

INSERT IGNORE INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
    (62682, 'spell_darkshore_greymist_detonation'),
    (62684, 'spell_darkshore_greymist_countdown');
