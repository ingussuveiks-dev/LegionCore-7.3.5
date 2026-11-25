# Artifact acquisition quest map guidance

The acquisition worklist and the November 18 route catalog agree on 188 quests across all 36 weapons. Before the migration, 183 of those quests had a worldserver POI definition, with 20 definitions lacking points. Eighty quest-and-index groups were shared by multiple definitions. The server uses `(QuestID, Idx1)` to retrieve coordinates, so that sharing can place distinct maps or floors at the same location.

The migration adds 128 coordinate rows across 36 quests, fills 18 empty definitions and separates 34 verified variants from the index their sibling still uses. Existing point rows stay in place. The restoration uses the local 26972 QuestPOIBlob and QuestPOIPoint relationships, signed XY, and the archived TDB 735.00 definitions to disambiguate matching variants. Reindexing requires the complete original definition and original point set to match, an existing sibling, and a vacant or identical target set. The Silver Hand quest 42120 already had the exact native Tirisfal XY, and its Area ID now matches that native point.

| Quest | Guidance added from local placements or route data |
| --- | --- |
| 40784 Its Rightful Place | Valorn's handoff at the first 101657 waypoint and the 101651 turn-in in Duskwood |
| 41015 Artifacts Need Artificers | Betild 102709 in Netherlight Temple |
| 41017 Empowering Your Artifact | Altar 246320 and Betild in Netherlight Temple |
| 41113 The Only Way to Travel | Meryl 102700 on the upper floor of the Hall of the Guardian |
| 42772 Sacred Ground | Three phase-matched Sorrow Hill gravestones 251336, 251340 and 251341 and quest receiver 108777. Native spells 216206, 216219 and 216222 reach credits for the three individual graves and the shared purification objective. |

Join the Dreamer and In Deep Slumber also receive Naralex anchors from scoped Tel’Andu Barrow Den spawns. Two old empty variants, 40935 index 3 and 42503 index 1, have no matching position in the examined sources and remain open. Both quests already have other map points. Historical entries for objectives 284218 in 42438 and 284358 in 42476 refer to objects different from the current live objectives. Their legacy geometry was left in place. The 188-quest count covers quests with at least one populated map marker. Individual objectives without an independent marker include phased scenario triggers, item use steps and historical definitions. Their locations and phase transitions require separate evidence.

MariaDB shadow application verified 10 added definitions, 128 new coordinates and the 34 reindexed rows. A repeat returned byte-equivalent definition and point rows. Guard fixtures kept a custom point set, an orphan reservation, a changed player condition, a changed original point set, an ambiguous empty index and a custom definition. The canonical updater applied migration 13 as RELEASED with SHA-1 `DB61F1651858238214639DE26CA5912A53B6B490`. The worldserver loaded 89,334 POI definitions and the owned process reached ready, then shut down normally with exit 0. Readback matched all 62 planned definitions, 128 coordinates, the Tirisfal area correction and populated markers for 188 quests.

Pre-test and post-test crash inventories stayed empty. No new server warning, error, assertion or DBErrors entry appeared. The canonical executable and protected files retained their hashes. No additional executable or isolated runtime was created. Local snapshots, shadow results, readback and startup diagnostics remain in the ignored `.codex/artifact-quest-map-points-20261125/` directory. Actual map and minimap rendering in the 7.3.5 client remains unverified.
