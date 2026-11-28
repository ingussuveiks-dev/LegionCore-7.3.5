-- Northwatch Ranger: use the ordered aggro chain also used by Palemane Poacher.
-- The imported no-link row repeats the movement stop performed by the chain root.
UPDATE `smart_scripts` AS shoot
SET shoot.`event_type` = 61
WHERE shoot.`entryorguid` = 39261 AND shoot.`source_type` = 0
  AND shoot.`id` = 1 AND shoot.`link` = 2 AND shoot.`event_type` = 4
  AND shoot.`event_flags` = 1 AND shoot.`event_phase_mask` = 0
  AND shoot.`event_chance` = 100 AND shoot.`Difficulties` = ''
  AND shoot.`action_type` = 11 AND shoot.`action_param1` = 6660 AND shoot.`target_type` = 2
  AND EXISTS (
    SELECT 1 FROM `smart_scripts` AS root
    WHERE root.`entryorguid` = 39261 AND root.`source_type` = 0 AND root.`id` = 0
      AND root.`link` = 1 AND root.`event_type` = 4 AND root.`event_flags` = 1
      AND root.`action_type` = 21 AND root.`action_param1` = 0 AND root.`target_type` = 1
  ) AND EXISTS (
    SELECT 1 FROM `smart_scripts` AS tail
    WHERE tail.`entryorguid` = 39261 AND tail.`source_type` = 0 AND tail.`id` = 2
      AND tail.`link` = 3 AND tail.`event_type` = 61 AND tail.`action_type` = 20
      AND tail.`action_param1` = 0
  );

DELETE duplicate_root FROM `smart_scripts` AS duplicate_root
JOIN `smart_scripts` AS root
  ON root.`entryorguid` = duplicate_root.`entryorguid` AND root.`source_type` = duplicate_root.`source_type`
 AND root.`id` = duplicate_root.`id` AND root.`link` = 1
 AND (root.`Difficulties`, root.`event_type`, root.`event_phase_mask`, root.`event_chance`, root.`event_flags`,
      root.`event_param1`, root.`event_param2`, root.`event_param3`, root.`event_param4`, root.`event_param5`,
      root.`action_type`, root.`action_param1`, root.`action_param2`, root.`action_param3`, root.`action_param4`,
      root.`action_param5`, root.`action_param6`, root.`target_type`, root.`target_param1`, root.`target_param2`,
      root.`target_param3`, root.`target_param4`, root.`target_x`, root.`target_y`, root.`target_z`, root.`target_o`)
   = (duplicate_root.`Difficulties`, duplicate_root.`event_type`, duplicate_root.`event_phase_mask`,
      duplicate_root.`event_chance`, duplicate_root.`event_flags`, duplicate_root.`event_param1`,
      duplicate_root.`event_param2`, duplicate_root.`event_param3`, duplicate_root.`event_param4`,
      duplicate_root.`event_param5`, duplicate_root.`action_type`, duplicate_root.`action_param1`,
      duplicate_root.`action_param2`, duplicate_root.`action_param3`, duplicate_root.`action_param4`,
      duplicate_root.`action_param5`, duplicate_root.`action_param6`, duplicate_root.`target_type`,
      duplicate_root.`target_param1`, duplicate_root.`target_param2`, duplicate_root.`target_param3`,
      duplicate_root.`target_param4`, duplicate_root.`target_x`, duplicate_root.`target_y`,
      duplicate_root.`target_z`, duplicate_root.`target_o`)
JOIN `smart_scripts` AS shoot
  ON shoot.`entryorguid` = root.`entryorguid` AND shoot.`source_type` = root.`source_type`
 AND shoot.`id` = 1 AND shoot.`link` = 2 AND shoot.`event_type` = 61
 AND shoot.`action_type` = 11 AND shoot.`action_param1` = 6660
WHERE duplicate_root.`entryorguid` = 39261 AND duplicate_root.`source_type` = 0
  AND duplicate_root.`id` = 0 AND duplicate_root.`link` = 0
  AND duplicate_root.`event_type` = 4 AND duplicate_root.`action_type` = 21;

-- Frozen Mountaineer: emote on summon. Movement is an independent 3s OOC
-- event which links to the 3s despawn. Dispatching it on summon would skip the wait.
UPDATE `smart_scripts` AS emote
SET emote.`link` = 0
WHERE emote.`entryorguid` = 41763 AND emote.`source_type` = 0
  AND emote.`id` = 0 AND emote.`link` = 1 AND emote.`event_type` = 54
  AND emote.`action_type` = 5 AND emote.`action_param1` = 20
  AND EXISTS (
    SELECT 1 FROM `smart_scripts` AS movement
    WHERE movement.`entryorguid` = 41763 AND movement.`source_type` = 0
      AND movement.`id` = 1 AND movement.`link` = 2 AND movement.`event_type` = 1
      AND movement.`event_flags` = 1 AND movement.`event_param1` = 3000 AND movement.`event_param2` = 3000
      AND movement.`action_type` = 46 AND movement.`action_param1` = 6
  ) AND EXISTS (
    SELECT 1 FROM `smart_scripts` AS despawn
    WHERE despawn.`entryorguid` = 41763 AND despawn.`source_type` = 0
      AND despawn.`id` = 2 AND despawn.`link` = 0 AND despawn.`event_type` = 61
      AND despawn.`action_type` = 41 AND despawn.`action_param1` = 3000
  ) AND NOT EXISTS (
    SELECT 1 FROM `smart_scripts` AS existing
    WHERE existing.`entryorguid` = 41763 AND existing.`source_type` = 0
      AND existing.`id` = 0 AND existing.`link` = 0
  );

-- Cowering Trooper: the rescue click gives the original credit and starts group
-- 1 speech. The existing TEXT_OVER row schedules despawn after the speech ends.
UPDATE `smart_scripts` AS speech
SET speech.`link` = 0
WHERE speech.`entryorguid` = 45910 AND speech.`source_type` = 0
  AND speech.`id` = 1 AND speech.`link` = 2 AND speech.`event_type` = 61
  AND speech.`action_type` = 1 AND speech.`action_param1` = 1 AND speech.`action_param2` = 3000
  AND EXISTS (
    SELECT 1 FROM `smart_scripts` AS rescue
    WHERE rescue.`entryorguid` = 45910 AND rescue.`source_type` = 0
      AND rescue.`id` = 0 AND rescue.`link` = 1 AND rescue.`event_type` = 73
      AND rescue.`action_type` = 33 AND rescue.`action_param1` = 45910 AND rescue.`target_type` = 7
  ) AND EXISTS (
    SELECT 1 FROM `smart_scripts` AS despawn
    WHERE despawn.`entryorguid` = 45910 AND despawn.`source_type` = 0
      AND despawn.`id` = 3 AND despawn.`link` = 0 AND despawn.`event_type` = 52
      AND despawn.`event_param1` = 1 AND despawn.`event_param2` = 45910
      AND despawn.`action_type` = 41 AND despawn.`action_param1` = 4000
  ) AND NOT EXISTS (
    SELECT 1 FROM `smart_scripts` AS existing
    WHERE existing.`entryorguid` = 45910 AND existing.`source_type` = 0
      AND (existing.`id` = 2 OR (existing.`id` = 1 AND existing.`link` = 0))
  );
