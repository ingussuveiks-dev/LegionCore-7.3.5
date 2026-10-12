# Aviash: recovered 7.3.5 landing point, flight still unresolved

The native landing destination is now identified. **The optional flight for quest 39387 is not fixed by this research commit.** No AI binding, route, spellclick, live database or server executable is changed.

## Confirmed native evidence

The installed `WorldSafeLocs.db2` contains both coordinates and explicit names:

| ID | Native name | Map | X | Y | Z | Orientation (degrees) |
| --- | --- | --- | --- | --- | --- | --- |
| 5363 | 7.0 Highmountain: zto - Aviash drop off point | 1220 | 4226.380371 | 4421.390625 | 663.388977 | 203.758484 |
| 5364 | 7.0 Highmountain: zto - Aviash drop off point (Witchqueen) | 1220 | 5139.839844 | 4445.250000 | 1022.679993 | 203.919006 |

These are extracted **7.3.5 records**, not coordinates inferred solely from a newer database. [TrinityCore's WorldSafeLocs import](https://github.com/TrinityCore/TrinityCore/blob/978e4f5736d289e2d7d12a6a878dacc108fd5496/sql/old/8.x/world/19071_2020_06_17/2019_10_26_01_world.sql#L2624) independently preserves the same labels and rounded coordinates. Convert `LocO` from degrees to radians before passing it to a server `Position`.

5363 is at Thunder Totem's lower approach, near Lasan 98773. It is distinct from normal taxi node 1719 on the upper platform. 5364 is the Witchqueen-side landing point; it is not the Thunder Totem destination for this quest. Contemporary [Blizzplanet coverage](https://warcraft.blizzplanet.com/the-skies-of-highmountain-highmountain-quests-alliance/) confirms that Aviash takes the player to Thunder Totem for Lasan's ceremony.

Existing spellclick 98777 / 194359, native summon 98778 / properties 3054, vehicle 4428 and seat 16028 remain the correct candidates. Properties 3054 have control 0/title 0; this core's ordinary summon branch does not automatically board the player. Live template 98778 has no AI/script or route. The missing behavior is boarding and flight control, not an established obsolete-ID replacement.

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

## Remaining work

Recover the original server movement spline from a matching sniff, or explicitly implement a reconstructed route and validate its clearance against the installed geometry. The confirmed landing point alone cannot establish that route. Boarding must complete before departure, and owner cancellation, dismount, mover/viewpoint restoration and retry need implementation and regression checks. Client flight/camera/pet portrait validation remains outstanding. The level-100 boost and custom ship scenario are untouched.
