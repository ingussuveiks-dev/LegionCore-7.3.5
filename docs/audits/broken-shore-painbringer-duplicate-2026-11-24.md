# Broken Shore overlapping Painbringers

The stage-six client screenshot showed Mo'arg Painbringers overlapping. The live map 1460 spawn table contained two same-entry, same-difficulty, same-phase records 294825 and 294827 for entry 92564 at 1254.48, 2438.92, 47.0961, orientation 2.82968. Both positions and facing match exactly within the database. No spawn-specific addon, movement override, formation, pool, event, linked respawn, conversation actor, waypoint or SmartAI reference was found for the duplicate. The separate 92564 spawn 294796 and the other same-name entries have distinct placements.

The world migration removes only the unlinked stationary duplicate at the matching placement. It preserves the original spawn, the other Mo'arg entries, and all existing native combat and scenario criteria. Entry 92564 has no Stage 6 death event in the instance script. A custom layout or protected spawn association makes the migration leave that row in place.

Read-only live data inspection and source review were used during the active player-led run. The ordinary world updater applies the migration on the next restart. Current loaded creatures retain their existing map population until restart. A subsequent client run can check the original spawn location.
