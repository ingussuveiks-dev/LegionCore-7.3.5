START TRANSACTION;

-- The scoped controller now produces the three-imp waves.
DELETE FROM smart_scripts
WHERE entryorguid = 115638 AND source_type = 0 AND id = 10 AND link = 0
  AND event_type = 0 AND event_phase_mask = 0 AND event_flags = 0
  AND event_param1 = 55000 AND event_param2 = 55000 AND event_param3 = 55000 AND event_param4 = 55000
  AND action_type = 11 AND action_param1 = 230165 AND action_param2 = 1 AND target_type = 1;

-- The native Imp still applies Shadow Shield. Its death only clears its own contribution.
DELETE FROM smart_scripts
WHERE entryorguid = 115642 AND source_type = 0 AND id = 2 AND link = 0 AND event_type = 6
  AND action_type = 28 AND action_param1 = 243027 AND target_type = 23;

-- The original two-second Translocate relocates the imp around its summoner.
INSERT INTO smart_scripts
(entryorguid, source_type, id, event_type, event_chance, event_param1, event_param2, event_param3, event_param4,
 action_type, action_param1, target_type, comment)
VALUES (115642, 0, 5, 0, 100, 10000, 16000, 14000, 22000, 11, 242950, 23,
 'Umbral Imp - In combat - Translocate near the summoner')
ON DUPLICATE KEY UPDATE entryorguid = VALUES(entryorguid);

COMMIT;
