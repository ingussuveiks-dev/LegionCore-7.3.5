-- The map 1494 encounter now protects both scripted health thresholds after scaling.
-- The old pre-scaling SmartAI action could set health to a single raw hit point.
DELETE FROM `smart_scripts`
WHERE `entryorguid` = 102785 AND `source_type` = 0 AND `id` = 8 AND `link` = 0
  AND `event_type` = 60 AND `action_type` = 42 AND `action_param1` = 1
  AND `action_param2` = 0 AND `target_type` = 1;
