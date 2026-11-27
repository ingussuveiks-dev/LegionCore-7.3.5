-- SmartAI processes event IDs in ascending order. Let a due Nether Burst use
-- the first free cast slot before Soulflame and Incinerating Blast refill it.
UPDATE `smart_scripts`
SET `id` = CASE `id` WHEN 1 THEN 16 WHEN 2 THEN 17 END
WHERE `entryorguid` = 108168 AND `source_type` = 0 AND `link` = 0 AND `event_type` = 0
  AND `action_type` = 11 AND ((`id` = 1 AND `action_param1` = 202420) OR (`id` = 2 AND `action_param1` = 202290));
