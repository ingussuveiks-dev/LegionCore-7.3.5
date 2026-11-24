-- An absent waypoint script clears Genn's roaming flag and invokes the
-- generic player-stop timer. Keep the action if its script has been restored.
UPDATE `waypoint_data_script`
SET `action` = 0
WHERE `id` = 439136 AND `point` = 6 AND `delay` = 0
  AND `action` = 344 AND `action_chance` = 100
  AND NOT EXISTS (SELECT 1 FROM `waypoint_scripts` WHERE `id` = 344);
