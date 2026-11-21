START TRANSACTION;

-- The three Smoldering Imps open the fight. Subsequent slots are in the mixed wave.
UPDATE smart_scripts
SET event_flags = 1
WHERE entryorguid = 115638 AND source_type = 0 AND id = 7 AND link = 0
  AND event_type = 0 AND event_phase_mask = 0 AND event_flags = 0
  AND event_param1 = 0 AND event_param2 = 0 AND event_param3 = 60000 AND event_param4 = 60000
  AND action_type = 80 AND action_param1 = 115638 AND action_param2 = 2 AND target_type = 1;

-- The original DBM module recorded first Imp Servant warnings at 11 and 14 seconds.
UPDATE smart_scripts
SET event_param1 = 11000, event_param2 = 14000
WHERE entryorguid = 115638 AND source_type = 0 AND id = 11 AND link = 0
  AND event_type = 0 AND event_phase_mask = 0 AND event_flags = 0
  AND event_param1 = 15000 AND event_param2 = 15000 AND event_param3 = 45000 AND event_param4 = 45000
  AND action_type = 80 AND action_param1 = 1156380 AND action_param2 = 2 AND target_type = 1;

COMMIT;
