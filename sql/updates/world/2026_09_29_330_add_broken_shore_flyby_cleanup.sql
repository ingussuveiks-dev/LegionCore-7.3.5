-- Sylvanas summons Winged Nightmares (92801) every three seconds on the
-- Horde finale ridge. Their non-repeating flight path ends at node 8 with
-- action 347, but that action is missing, so completed flybys accumulate.
-- Despawn the source creature shortly after it reaches the final node.
-- Keep any existing implementation of 347 and require the matching path.
INSERT INTO `waypoint_scripts`
    (`id`, `delay`, `command`, `datalong`, `datalong2`, `dataint`,
     `x`, `y`, `z`, `o`, `guid`)
SELECT 347, 0, 18, 1000, 0, 0, 0, 0, 0, 0,
    (SELECT COALESCE(MAX(`guid`), 0) + 1 FROM `waypoint_scripts`)
WHERE NOT EXISTS (SELECT 1 FROM `waypoint_scripts` WHERE `id` = 347)
    AND EXISTS (SELECT 1 FROM `waypoint_data_script`
        WHERE `id` = 439155 AND `point` = 8 AND `action` = 347);
