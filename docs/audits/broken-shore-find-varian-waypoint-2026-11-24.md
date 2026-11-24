# Broken Shore Find Varian waypoint pause

The replay with the previous combat cleanup reached Find Varian and the formation stopped again at approximately 640, 2046. It had begun path 439136. The live waypoint row at point 6 has delay zero and action 344 with chance 100. The loaded waypoint script table has no row for 344.

The waypoint generator clears the roaming movement flag before trying to start the action. The missing action schedules nothing, so the same arrival enters the configured `Creature.MovingStopTimeForPlayer` wait of 180,000 ms. This explains the earlier client observation that the group eventually resumed after a long pause.

The migration removes the missing action reference only from the matching point. An installed action 344, custom timing on that point, other waypoints, the scene near the end of the first path and subsequent stages retain their existing behavior. The earlier scoped combat cleanup remains in place.

The source trace and read-only data inspection identify the no-op and timer. The normal world updater applies the migration on restart. Separate automated and agent-owned gameplay checks are omitted for the active user-led run. Confirm the group passes point 6 without the three-minute wait and proceeds through the scene into Stage 5.
