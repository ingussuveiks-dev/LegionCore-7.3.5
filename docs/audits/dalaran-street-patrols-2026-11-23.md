# Dalaran street patrols

The player saw Halga Steelbarrow and Armond Thaco walking inside the street surface on map 1220. Halga (entry 97490, spawn 272903) uses template path 974900. Armond (entry 108628, spawn 272902) uses spawn path 272902. The unused path 272903 is separate.

The waypoint movement generator sends a straight spline between consecutive database nodes, including the last-to-first leg. On the extracted Legion city WMO, Armond's closing shortcut was up to 3.99 yards below model geometry and Halga's longest sinking segment was 2.01 yards below the street. Original waypoint heights by themselves were insufficient because the street changes height between them.

Migration 2026_11_23_08 keeps Armond's return along his outbound street and keeps Halga's circuit with intermediate positions derived from the extracted walkable mesh and city floor. Geometry readback covered the new segments, loop closures and approaches from each original spawn, sampling at intervals of up to 0.21 yards. The largest sampled depth below the WMO was 0.023 yards for Armond and 0.062 yards for Halga. Existing spawn locations, movement types and other routes are preserved. The migration only replaces the two exact original database paths.

After the user-owned worldserver stopped, the migration was applied to the runtime world database. All 48 points on path 272902 and 74 on 974900 matched the migration at database float precision. The separate path 272903 still has its original six points. The updater will register the migration on the next startup. No separate automated or agent-owned startup test was run for the active gameplay session. Both NPCs' appearance over a full circuit remains to be checked in the client.
