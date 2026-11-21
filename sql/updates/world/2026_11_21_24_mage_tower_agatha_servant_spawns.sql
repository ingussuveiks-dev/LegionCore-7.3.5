START TRANSACTION;

-- Preserve the original timed pair and emote. Ask Agatha for each phase-specific deployment.
UPDATE smart_scripts
SET action_type = 45, action_param1 = 4001, action_param2 = id, target_type = 1,
    target_x = 0, target_y = 0, target_z = 0, target_o = 0,
    comment = 'Agatha - Summon an Imp Servant from the current side of the arena'
WHERE entryorguid = 1156380 AND source_type = 9 AND id IN (0, 1)
  AND link = 0 AND event_type = 0 AND event_param1 = 0 AND event_param2 = 0
  AND action_type = 12 AND action_param1 = IF(id = 0, 115719, 115643) AND action_param2 = 8 AND target_type = 8;

COMMIT;
