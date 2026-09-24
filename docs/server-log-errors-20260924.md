# Time synchronization and corpse cleanup, 2026-09-24

## Evidence from the last server run

`build-extractors/bin/Release/logs/Server.log` contains two discarded-time-sync
errors at 13:32:13 (client maximum 263, server next index 265 and then 0),
and four missing corpse/bones errors at 13:37:28 (GUID lows 1, 4, 2, 3).

## Fixes

- The discard handler previously compared the client's maximum against the
  next outgoing movement/time-sync index, then reset that shared index to zero.
  It now retires only the discarded outstanding requests and preserves the
  outgoing counter. Duplicate notifications are harmless, and notifications
  outside the issued sequence range cannot clear new-map requests.
- A time-sync response updates client time only if it matches an outstanding
  request. Stale/unknown responses cannot consume the next valid request.
  A known later response also retires older requests with missing replies.
- `ResetTimeSync` clears pending old-map requests before sequence numbers are
  reused by the existing non-seamless teleport initialization.
- `Map::AddObjectToRemoveList` calls `CleanupsBeforeDelete`, which calls
  `Corpse::RemoveFromWorld` and unregisters the object from ObjectAccessor.
  Looking it up again while draining the removal queue therefore fails.
  The queue now passes its owned corpse pointer directly to `RemoveFromMap`.
  This completes removal instead of leaking the object or finding a different
  object reusing its GUID. Resurrectable corpses still follow the separate
  world-grid/cache path in `CorpseGridReset`.

## Verification

- `tools/tests/TimeSyncTest.cpp` compiled with MSVC and passed: the logged
  263/265 case, duplicate discard, stale/unknown/duplicate responses, missing
  older response, old-map notification, unissued index and 32-bit wraparound.
- Release worldserver build passed in `build-extractors/bin/Release`.
- Reviewed corpse cleanup through the grid unloader, removal queue and final
  map deletion, including the separate cached resurrectable-corpse path.
- No server was started. Confirm in the next gameplay run that map transfer
  and later corpse/bones cleanup produce neither reported error.
