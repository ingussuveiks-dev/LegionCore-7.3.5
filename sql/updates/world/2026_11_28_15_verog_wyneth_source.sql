-- The Legion version of 851 starts the encounter from Wyneth (34846).
-- These death rows are remnants of the pre-Cataclysm random Kolkar summon.
DELETE FROM `smart_scripts`
WHERE `source_type` = 0 AND `event_type` = 6 AND `target_type` = 1
  AND `event_param1` = 0 AND `event_param2` = 0
  AND ((`entryorguid` = 3275 AND `id` = 1 AND `link` = 2
        AND `event_chance` = 3 AND `action_type` = 1 AND `action_param1` = 0)
    OR (`entryorguid` = 3274 AND `id` = 1 AND `link` = 2
        AND `event_chance` = 20 AND `action_type` = 12 AND `action_param1` = 3395 AND `action_param2` = 8)
    OR (`entryorguid` = 3397 AND `id` = 2 AND `link` = 3
        AND `event_chance` = 10 AND `action_type` = 12 AND `action_param1` = 3395 AND `action_param2` = 8));

-- Remove the now unused speech continuations of the two old summons.
DELETE line FROM `smart_scripts` line
LEFT JOIN `smart_scripts` root ON root.`entryorguid` = line.`entryorguid`
  AND root.`source_type` = line.`source_type` AND root.`link` = line.`id`
WHERE line.`source_type` = 0 AND line.`event_type` = 61 AND line.`link` = 0
  AND line.`action_type` = 1 AND line.`action_param1` = 0 AND line.`target_type` = 1
  AND ((line.`entryorguid` = 3274 AND line.`id` = 2 AND line.`event_chance` = 100)
    OR (line.`entryorguid` = 3397 AND line.`id` = 3 AND line.`event_chance` = 10))
  AND root.`entryorguid` IS NULL;
