-- The command ship has walkable decks and ramps in the map 1666 navmesh.
-- These combat actors use ground movement on those surfaces.
INSERT INTO `creature_movement_override` (`SpawnId`, `Swim`, `Flight`)
SELECT c.`guid`, 0, 0
FROM `creature` AS c
JOIN `creature_template_movement` AS m ON m.`CreatureId` = c.`id`
WHERE c.`map` = 1666 AND m.`Ground` = 1 AND m.`Swim` = 1 AND m.`Flight` = 2
  AND (c.`guid`, c.`id`) IN (
    (363439, 120866), (363440, 120866), (363447, 120866), (363452, 120866),
    (363441, 120869), (363443, 120869), (363448, 120869), (363454, 120869), (363457, 120869),
    (363444, 120870), (363449, 120870),
    (363445, 120795), (363451, 120795), (363459, 120795),
    (363460, 120746))
ON DUPLICATE KEY UPDATE
    `Swim` = COALESCE(`creature_movement_override`.`Swim`, VALUES(`Swim`)),
    `Flight` = COALESCE(`creature_movement_override`.`Flight`, VALUES(`Flight`));
