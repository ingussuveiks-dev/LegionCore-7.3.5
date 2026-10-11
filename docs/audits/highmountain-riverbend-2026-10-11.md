# Riverbend opening — 2026-10-11

Scope: 42104 The Underking Comes, 39025 Grasp of the Underking, 39026 The Drogbar and 39043 Bitestone Enclave. Migration `2026_10_11_00_highmountain_riverbend.sql`; handler `highmountain_riverbend.cpp`. Native records and hashes are in `highmountain-riverbend-native-2026-10-11.json`.

## Native behavior and repairs

42104 requires **ten evacuations**, not ten kills. Installed DB2 links aura 210983 to OverrideSpellData 702, whose action is Horn of Jale 210891. Its dummy area effect has radius 12 yards. The existing area row enables this button only while the quest is incomplete in Riverbend (7709); it is retained. Jale 97662 starts the quest and Spiritwalker Ebonhorn 93805 ends it. His existing reward action plays scene 192268/package 1092, which is also retained.

The former 100520 SmartAI handlers accepted any 210891 hit, credited once globally and despawned after five seconds without moving. The new AI validates quest, living player/NPC, area/map, visibility and range. Each citizen counts at most once per player during that spawn. Nearby eligible group members retain shared credit, with deduplication when another member sounds the horn. Another unrelated player can still warn the departing citizen. Only objective 100520 of quest 42104 advances; evacuation does not increment kill achievements. Credit represents the warning, as in the existing spell-hit implementation, rather than arrival at the refuge.

The citizen speaks using its existing broadcast texts, runs toward the existing local Ebonhorn and despawns two seconds after arrival. A sixty-second bound prevents a failed path from leaving a permanently active evacuee. Abandoning/reaccepting does not replay deferred credit (there are no player callbacks); already warned citizens remain evacuated until their ordinary respawn. Persisted objective progress is read on every warning, so relogging does not reset the count. Seventeen citizen spawns exist for a ten-citizen objective.

39025 previously searched for three entry IDs within five yards of the hand and could award a nearby captive's credit without checking the vehicle passenger. Its replacement retains the `npc_grasp_of_underking_quest` binding, but checks the live, owned seat-zero passenger of native vehicle 4397/seat 15974 against the correct existing grasp site. It awards credit only after successful vehicle exit and rejects repeat clicks, empty seats, unrelated passengers and ineligible players. Existing nearby group credit and rescue dialogue are retained. The hand's original three coordinates identify Oro, Jale and Oakin with a small float tolerance; no new placement was invented.

The old one-time `boarded` latch could remain set after a failed summon. The new AI retries missing summons/boarding every two seconds and recreates the captive on reset/respawn. Removed the three competing SmartAI boarding timers that cast 46598 on a nearby hand, including Oakin's repeating timer. The hand now owns boarding. Other captive dialogue/data-event scripts remain intact.

39043 required five Rockbeater credits and five Manaspine Basilisks. TDB735 objectives 283671 and 283672 require **three each**. Restored only those amounts; no target IDs were substituted. A scoped login hook clamps surplus saved progress to the restored requirement and rechecks completion. Without this, an old incomplete 3/5 + 3/5 save could remain stuck: the kill handler skips slots already at the new maximum. This only marks an earned quest complete; it does not grant its reward.

## Audited without data changes

- 39026 requires eight credit 102274 kills. Ten actual 102274 spawns and two 103067 Slingers with KillCredit2=102274 are present. Valid 98–110 scaling exists for these and both 39043 target types.
- Six Rockbeaters and sixteen basilisks are available for 39043; existing alternative drogbar credit aliases remain intact.
- The four starter/ender relations exist. The parallel 39025/39026 offers both have prerequisite 42104. The existing 39026 prerequisite for 39043 and onward link to 39027 are retained. The contemporary guide lists the same prerequisite; an unproved requirement to reward both parallel quests was not added.
- All three hands exist in phase 5645, backed by Highmountain phase definition 9. Definitions 8–11 have no source-23 conditions in this database, so their phases are not blocked by missing quest conditions. This confirms availability, not retail-perfect phase transitions or scene visuals.

Behavior cross-checks: [Legion-era Highmountain route](https://www.wow-pro.com/highmountain-neutral/) describes the horn, parallel rescue/combat tasks and later enclave; [Blizzard forum reports](https://us.forums.blizzard.com/en/wow/t/quest-bug-grasp-of-underking/732097) describe releasing the captives by clicking the hands. Quest IDs, objective counts and spell/vehicle identities are taken from local version-specific data, not modern scaling values.

## Validation and limits

`Test-HighmountainRiverbendNative.py` verifies four native quests, seven objectives and the DB2 horn/action bar/radius, vehicle seat and reward scene. `Test-HighmountainRiverbendData.ps1` applies the SQL twice to temporary tables, checks the native objectives, bindings, spawns, phases, existing chain and preservation of unrelated records; `-Live` checks installation. `Test-HighmountainRiverbend.ps1` compiles the actual production handlers with adapters and exercises invalid/range/phase horn casts, group and per-player deduplication, completion at ten, retained progress after reconnect, wrong/empty vehicle seats, failed spawn/exit retry, repeat clicks and respawn.

An additional read-only MMAP probe found complete paths from 15 of the 17 existing citizen positions to Ebonhorn at (3708.77, 4962.49, 671.358). GUIDs **284194 and 284198** have no nearest navigation polygon within the probe's (3,4,3) search extent at their recorded positions. Their exact walking behavior is not verified; no speculative relocation or navmesh edits were made. The warning objective is still credited, and the bounded despawn handles failure to arrive. Actual client movement, including the existing waypoint movement of 284198, needs inspection.

A client playthrough is still required for horn appearance/removal on completion or area exit, citizen running, the reward scene, the three hand/captive animations, objective UI updates from 5 to 3 for an existing quest log, and party/abandon/relog behavior. These server tests do not verify those visuals. The next full content audit is 39027 Dargrul and the Hammer, beyond this batch.

Release runtime belongs in `build-extractors/bin/Release`. The custom boost/ship/Broken Shore, global criteria, camera and pet implementations were not changed.
