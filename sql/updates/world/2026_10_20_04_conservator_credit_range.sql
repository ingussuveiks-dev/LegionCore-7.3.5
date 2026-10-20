-- A ranged warlock can stand beyond 30 yards while the pet kills Conservator.
-- PLAYER_DISTANCE selects players around the dying boss, not around its killer;
-- owner-credit normalization cannot recover a player absent from that list.
-- Keep the death event, criterion, nearby-player targeting and linked visuals.
UPDATE smart_scripts SET target_param1=100
WHERE entryorguid=101257 AND source_type=0 AND id=1 AND event_type=6
  AND action_type=205 AND action_param1=92 AND action_param2=47985
  AND target_type=18 AND target_param1=30;
