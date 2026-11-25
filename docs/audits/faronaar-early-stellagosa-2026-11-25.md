# Early Stellagosa at Traitor's Overlook

The client showed Stellagosa while the Nightglaive objective in Dark Revelations was `0/1`. The only placed 90623 at Traitor's Overlook is spawn 338234. It has spell-click 178923, seat-zero passenger 90624, and a visibility phase available before the kill. The existing personal return vehicle 90982 has a separate entry and rider. Its native 178923 summon is already gated on the credited Nightglaive objective in `UpdateRide`.

During this report the user killed Nightglaive and confirmed that the personal return ride boarded automatically without clicking the earlier dragon. Read-only live progress showed both the actual kill and boarding objectives credited. The arrival objective was still pending at inspection.

The migration removes only the earlier placed 90623 duplicate with its exact map, area and phase fingerprint. It retains the native summon and actual boarding route, the optional flight and the existing on-foot return, phase definitions, creature templates and other Stellagosa placements. Before applying the migration, read-only live validation found one exact matching row, the only spell-click source for 178923 and no spawn-addon, formation, linked-respawn, game-event or pool references to that placement. This data repair uses the already deployed personal ride implementation.

After the user's restart, MariaDB records the update as RELEASED with SHA-1 `4C371D5048AC2ACED55D2298AA5D98D2FB9A557A`, matching the file. A live readback found zero rows for spawn 338234, an empty DBErrors.log and no canonical crash reports. Pre-kill visibility after applying the update still needs client confirmation.
