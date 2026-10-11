# Step into the Dark / Unexpected Allies

## Findings

Step into the Dark (40229) has no numeric objective in TDB735. Its live
starter is companion Navarrogg 96453 and its ender is Navarrogg 95392 at
Stonedark Grotto, spawn 340565. The previous Blood Debt change already
restored the correct 40229 -> 39456 link and required Blood Debt rewarded.
These data are retained and covered by the SQL test.

Unexpected Allies (39456) had three SmartAI gossip actions that immediately
credited 99619, 99624, then 99619 again. Because 99619 is shared by two
objectives, that completed the whole scene without escorting or witnessing
anything. The phase definition for Navarrogg's phase 6625 had no quest
conditions and existed only under Highmountain (7503), not Thunder Totem
(7731). PhaseMgr evaluates definitions for the current zone.

## Replacement sequence

- The static Navarrogg 99619 at the northwest entrance grants only meeting
  objective 280678 on a valid six-yard approach/interaction. Quest, life,
  phase, visibility, line of sight and transport state are checked.
- Menu 18971 option 0 creates a personal Navarrogg 99624. Both entries use
  native model 69863. Repeated clicks find the existing owner-specific actor;
  allocation failure leaves the interaction retryable.
- Navarrogg walks to (4083.05, 4388.5, 670.709), beside the existing Mayla
  spawn. A successful movement callback at that location is required before
  dialogue. The actual installed ground navmesh connects the start and end
  through 27 polygons. The native start Z is 1.72 m below the nearest ground
  polygon; the probe allows 2 m of mesh height tolerance and preserves the
  existing spawn coordinates.
- In front of a living, visible, same-phase Mayla, Navarrogg and Mayla
  exchange the local 7.3.5 BroadcastText pledge lines. Dialogue is sent only
  to the scene owner; shared Mayla is neither moved nor re-scripted. Mayla
  entries 93826 and 108434 are supported.
- Only after the final response and wait are hidden optional objective
  280680 and required witness objective 280843 credited, in that order.
  Credits use objective IDs, not ambiguous ObjectID 99619. Completion can
  occur once both meeting and witnessing are complete.
- Abandon/reaccept expires the old quest lifetime. Death, transport/map/
  phase changes, separation beyond 40 yards, missing Mayla or a two-minute
  movement timeout cancel without witness credit. Dialogue requires the
  owner within 15 yards of both actors. Return to entrance Navarrogg and
  select the option again to retry; a pending-despawn actor does not block it.
- Phase 6625 is now explicitly gated to incomplete/complete 39456 in both
  relevant zones. The gossip option requires an incomplete quest.

The pledge text comes from local BroadcastText 101325/101326/101327,
102106/102107/102108/102109, also consistent with the contemporary
[Unexpected Allies recording](https://warcraft.blizzplanet.com/unexpected-allies-highmountain-quests/).
The sequence is a server implementation, not a recovered client scene:
two seconds after arrival, then five seconds between dialogue stages.
No unverified spell ID, teleport, extra chief spawn or ritual visual is
invented. Other chiefs' responses and exact retail choreography are not
reconstructed. Client presentation and a full in-game playthrough remain
unverified.

## Native objectives and chain

| Objective | Type | Storage | ObjectID | Flags | Purpose |
| --- | ---: | ---: | ---: | ---: | --- |
| 280678 | 3 | 0 | 99619 | 0 | Meet Navarrogg |
| 280843 | 0 | 1 | 99619 | 3 | Witness ritual, required |
| 280680 | 0 | 2 | 99624 | 12 | Hidden optional wait |

The native comparison and archive hash are in the adjacent JSON. Mayla
93826/108434 are the native enders. RewardNextQuest is zero for 39456: this
finishes the Bloodtotem branch, so no new forced follow-up was added.
Boost, ship, scenario criteria, pet, camera and generic quest production
handlers are unchanged.

## Validation

- `Test-HighmountainUnexpectedAlliesNative.py`: original objectives, equal
  Navarrogg models, both enders, 40229 link and native branch end.
- `Test-HighmountainUnexpectedAllies.ps1`: compiled production sequence and
  quest-lifetime helper; two owners, no instant completion, exact objective
  IDs, valid arrival, full timed dialogue, duplicate/failed summons, retry,
  path timeout, missing Mayla, abandon/reaccept and death/map/phase/range exits.
- `Test-HighmountainBloodtotem.ps1`: production CanCompleteQuest with all
  eight combinations of native objectives, plus prerequisite reward checks.
  Meeting alone and the optional wait cannot complete 39456.
- `Test-HighmountainUnexpectedAlliesGeometry.ps1`: actual Release navmesh
  libraries and installed data, ground path reaches the destination.
- `Test-HighmountainUnexpectedAlliesData.ps1`: migration twice on temporary
  tables, objective/model parity, source and ender spawns, conditions across
  both zones, removal of instant credit, dialogue bindings and preservation
  of unrelated AI/SmartAI/text/phases. `-Live` checks deployed data read-only.
- Release scripts compile; final deployment uses the canonical Release
  runtime and ordinary SQL updater. Client confirmation remains required.
