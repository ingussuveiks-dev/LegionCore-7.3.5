# Broken Shore Arganoth escort admission

After Yim disembarked on the Broken Shore, the server logged two attempts to start Dread Commander Arganoth's active escort. The failures occurred before the Find Varian and Destroy the Portal stages. The earlier `InstanceMap::CanEnter` duplicate admission occurred before beach disembarkation. Its initiating caller remains unconfirmed.

Arganoth's reset and player line-of-sight callbacks could both start the same escort. Genn's action could also reset Arganoth's event timers and summons while the escort was active. The scenario AI now checks the escort state before either start and before handling Genn's reset request. The original first start, waypoint path, and death progression are retained.

The Release build was compiled and linked from the updated scripts library. The worldserver link map contains the Arganoth and Alliance heroes AI symbols from `broken_islands.obj`. The active user-led workflow leaves startup and scenario traversal to the user. The existing duplicate admission and escort messages remain in the server log.
