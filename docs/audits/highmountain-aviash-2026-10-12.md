# Aviash: reconstructed flight to the native 7.3.5 landing point

The optional Aviash ride for quest 39387 now has owner-specific boarding, flight and native vehicle exit handling. Its destination is the explicitly named 7.3.5 landing point. **The intermediate route is reconstructed, not recovered from a retail movement sniff.** The earlier research commit `cb02b3a7` only identified the destination; this implementation adds the missing behavior.

## Confirmed native evidence

The installed `WorldSafeLocs.db2` contains both coordinates and explicit names:

| ID | Native name | Map | X | Y | Z | Orientation (degrees) |
| --- | --- | --- | --- | --- | --- | --- |
| 5363 | 7.0 Highmountain: zto - Aviash drop off point | 1220 | 4226.380371 | 4421.390625 | 663.388977 | 203.758484 |
| 5364 | 7.0 Highmountain: zto - Aviash drop off point (Witchqueen) | 1220 | 5139.839844 | 4445.250000 | 1022.679993 | 203.919006 |

These are extracted **7.3.5 records**, not coordinates inferred solely from a newer database. [TrinityCore's WorldSafeLocs import](https://github.com/TrinityCore/TrinityCore/blob/978e4f5736d289e2d7d12a6a878dacc108fd5496/sql/old/8.x/world/19071_2020_06_17/2019_10_26_01_world.sql#L2624) independently preserves the same labels and rounded coordinates. Convert `LocO` from degrees to radians before passing it to a server `Position`.

5363 is at Thunder Totem's lower approach, near Lasan 98773. It is distinct from normal taxi node 1719 on the upper platform. 5364 is the Witchqueen-side landing point; it is not the Thunder Totem destination for this quest. Contemporary [Blizzplanet coverage](https://warcraft.blizzplanet.com/the-skies-of-highmountain-highmountain-quests-alliance/) confirms that Aviash takes the player to Thunder Totem for Lasan's ceremony.

Existing spellclick 98777 / 194359, native summon 98778 / properties 3054, vehicle 4428 and seat 16028 are retained. Properties 3054 have control 0/title 0; this core's ordinary summon branch does not automatically board the player. Before this repair, template 98778 had no AI/script or route. The missing behavior was boarding and flight control, not an established obsolete-ID replacement.

## Search and validation

`tools/tests/Test-HighmountainAviashNative.py` reads the extracted DB2 without hotfix overlays and checks the landing names/coordinates, summon, properties, vehicle and seat. The adjacent JSON records the exact rows, file hashes and search counts. Run normally to compare with saved evidence; `--write-evidence` explicitly regenerates it.

The route search joined all 52,164 `PathNode.LocationID` references to `Location` and found no node within 100 m (3D) of either the spellclick spawn or Thunder Totem landing. Searching all 73,104 `TaxiPathNode` rows, restricted to map 1220, found no node within 70 m of either endpoint. The sparse `SceneScriptText` catalog was validated and searched for Aviash, 39387, 98778, 194359 and the quest title, without matches. These bounded negative results do not prove that no unrelated or unnamed client record could be involved.

Local TrinityCore, DestinyCore, LegionPlayerBot and tcdevemu Legion source searches, TDB735/current archived waypoint and SmartAI searches, and scoped GitHub searches did not yield the original flight spline. Indexed web material identifies the ride but does not establish its intermediate world coordinates or boarding/dismount timing.

The existing `HighmountainSkyhornRitualGeometry.cpp` probe, linked against the actual Release navigation libraries, was also run with these two endpoints:

```text
4226.38037109375 4421.390625 663.3889770507812
4171.37 4398.75 660.976
```

It found nearby ground polygons and a complete **24-polygon walking route** from the native landing point to static Lasan. This verifies ground reachability only, not flight clearance, dismount animation or camera behavior. To reproduce, build the probe with `Test-HighmountainSkyhornRitualGeometry.ps1`, then run `build-extractors/runtime-test/highmountain-skyhorn-ritual/AudienceGeometry.exe` with the canonical runtime directory and a file containing the two lines above; put the runtime directory on `PATH` for its DLLs.

## Implementation

- `highmountain_aviash.cpp` binds only summoned 98778. Native spellclick and summon effects remain intact. A spell check and summon guard require the incomplete quest, living nearby owner, no flight/vehicle/transport/combat, line of sight and no active ride. SQL adds the matching quest condition to spellclick 98777/194359.
- The summon is placed 3 m above the existing source spawn, clear of the surrounding rock, before boarding. Flight starts only after both seat 0 occupancy and the player's boarding spline are confirmed. Relocation and `SetClientControl(bird,false)` precede movement; dismount restores the player's mover/viewpoint.
- Eight linear `MovePoint` segments at 18 m/s use `highmountain_aviash_route.h`. The initial lower route failed collision checks and was raised. Progress requires the expected movement callback at the actual node. Timeout cannot act as successful arrival.
- At the last node, 2 m above native 5363, `CalcExitVehiclePos` supplies the native ground location. `ExitVehicle` runs the ordinary transport-exit spline; the actor remains until that spline finishes. Normal arrival does not teleport or award quest credit. Lasan's existing ritual still owns its objectives.
- Duplicate clicks cannot create another active ride. State is keyed by owner GUID with quest-lifetime expiry and protected for map-thread callbacks. Death, abandonment/reaccept, missing actor, phase loss, movement/boarding timeout and manual dismount cancel the ride. After exit, interrupted riders return to the source ground point, allowing retry. An unrelated vehicle or externally requested teleport is not overridden.
- Logout clears the ride and saves its safe recovery destination. This core calls `OnLogout` after its ordinary save, so the cleanup explicitly saves again after setting the near-teleport destination. The normal in-session exit waits for its native spline; logout cannot wait for client acknowledgement.
- No boost, custom ship scenario, shared vehicle core, pet lookup, ritual objectives or other Aviash variants are changed.

## Repair validation

- `Test-HighmountainAviashNative.py`: version-specific destination labels/coordinates, summon/vehicle/seat and the original bounded route search. `flight_path_recovered: false` in the evidence JSON remains accurate: the implemented route is reconstructed.
- `Test-HighmountainAviashGeometry.ps1`: compiles against actual Release `GridMap` and VMAP libraries, samples every segment at <= 1 m spacing across a 2 m corridor, and checks model collision rays through the bird/rider corridor. All eight segments pass **10,539 terrain samples**; minimum sampled terrain clearance is **1.128 m** on final approach. It also verifies the code's native landing coordinates/orientation and ground navigation: **24 polygons** to Lasan, **2 polygons** from the source recovery point to the nearby native Witchqueen landing. Static collision checks do not model animated wings, dynamic actors or the client's camera.
- `Test-HighmountainAviash.ps1`: compiles actual production handlers with fixtures. Covers delayed/failed boarding, actual node arrival, independent owners, duplicates, interrupted/normal native exit, retry, quest lifetime, death/phase/visibility loss, missing actor, flight/exit timeout, logout saved recovery and external transfer. The fixture does not simulate client rendering.
- `Test-HighmountainAviashData.ps1`: twice-applied temporary migration, native spellclick/vehicle, source coordinates, quest condition, script bindings, ritual preservation and unchanged unrelated rows. `-Live` checks deployment.
- `Test-WarlockRegression.ps1`: existing Burning Rush and scripted mover resynchronization regressions pass.
- Release scripts compilation passes. Final handoff uses the canonical `build-extractors/bin/Release` runtime.

Client flight, camera, dismount, pet portrait and the complete ride-to-ritual playthrough remain unverified. A matching retail movement sniff could later replace the reconstructed route without changing the established IDs or landing destination.
