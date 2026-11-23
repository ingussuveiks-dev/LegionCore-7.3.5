-- The map 1666 ground-side fallback has the same native destination jump as the shore gate.
-- Reuse the existing lower-deck landing for the otherwise unresolved 236762 cast.
START TRANSACTION;

INSERT INTO `spell_target_position`
    (`id`, `target_map`, `target_position_x`, `target_position_y`, `target_position_z`, `target_orientation`)
SELECT 236762, `target_map`, `target_position_x`, `target_position_y`, `target_position_z`, `target_orientation`
FROM `spell_target_position`
WHERE `id` = 236671 AND `target_map` = 1666
  AND NOT EXISTS (SELECT 1 FROM `spell_target_position` WHERE `id` = 236762);

UPDATE `gameobject_template` SET `ScriptName` = 'go_bs_demoic_gates'
WHERE `entry` = 267956 AND `type` = 10 AND `Data10` = 236762 AND `ScriptName` = ''
  AND EXISTS (SELECT 1 FROM `gameobject` WHERE `guid` = 134451 AND `id` = 267956 AND `map` = 1666)
  AND EXISTS (SELECT 1 FROM `spell_target_position` WHERE `id` = 236762 AND `target_map` = 1666);

COMMIT;
