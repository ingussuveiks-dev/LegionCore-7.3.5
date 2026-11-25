-- These Deliverance Point laborers stand on the model of gameobject 267785.
-- The entrance banners also belong to that model. Use its event lifecycle.
INSERT INTO `game_event_creature` (`eventEntry`, `guid`)
SELECT 400, c.`guid`
FROM `creature` c
JOIN `game_event_creature` victoria
    ON victoria.`guid` = 12742696 AND victoria.`eventEntry` = 400
JOIN `game_event_gameobject` tower
    ON tower.`guid` = 126920 AND tower.`eventEntry` = 400
WHERE c.`map` = 1220 AND c.`zoneId` = 7543 AND c.`areaId` = 8554
  AND c.`MovementType` = 0
  AND (
      (c.`PhaseId` = '7043' AND (
          (c.`id` = 119462 AND c.`guid` IN (12672908, 12672909, 12672914))
          OR (c.`id` = 120818 AND c.`guid` IN (12672918, 12672919))
          OR (c.`id` = 120819 AND c.`guid` IN (12672926, 12672927))
      ))
      OR (c.`PhaseId` = '7050' AND c.`id` = 118552 AND c.`guid` IN (12673225, 12673226))
  )
  AND NOT EXISTS (SELECT 1 FROM `game_event_creature` existing WHERE existing.`guid` = c.`guid`);
