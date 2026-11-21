START TRANSACTION;

-- The scoped Agatha AI now requests the native 100-energy cast.
DELETE FROM smart_scripts
WHERE entryorguid = 115638 AND source_type = 0 AND id = 13 AND link = 0
  AND event_type = 0 AND event_phase_mask = 0 AND event_flags = 0
  AND event_param1 = 50000 AND event_param2 = 50000 AND event_param3 = 50000 AND event_param4 = 50000
  AND action_type = 11 AND action_param1 = 243114 AND action_param2 = 0 AND target_type = 1;

COMMIT;
