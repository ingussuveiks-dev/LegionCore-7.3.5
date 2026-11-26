-- Thoralius provides the Incense Burner. Quest acceptance does not finish the vision.
DELETE FROM `smart_scripts`
WHERE `entryorguid` = 42651 AND `source_type` = 0 AND `id` = 0 AND `link` = 0
  AND `event_type` = 19 AND `event_param1` = 26320
  AND `action_type` = 15 AND `action_param1` = 26320 AND `target_type` = 18;
