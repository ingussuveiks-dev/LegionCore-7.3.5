# Highmountain: Dargrul and the Hammer (39027)

## Evidence and defects

Compared the installed 7.3.5 DB2 files, the archived TDB735 build-25549 quest records, live world data, and the production spell/vehicle/SmartAI handlers. Reproducible source hashes and records are in `highmountain-dargrul-native-2026-10-11.json`.

- Native objectives: confront Dargrul 94610, follow Oro 97871, TALKTO/escape grasp 97781, leave the cave 93805. Follow and escape have native flags 12 (hidden/optional); the exit must still be gated by these actual story steps.
- Live event object 604 immediately credited every nearby player and signalled shared, nonrepeatable Dargrul/Ularogg SmartAI actors. Its conditions used source type 10 (scene), although this is a source-type-13 event object. No follow prerequisite was applied.
- Event object 543 had no conditions on its cave-exit credit. Its separate scene 6409 belongs to quest 41764; that condition also used the wrong source type.
- The summoned Oro (97552) credited waypoint 5 and disappeared at waypoint 6 of a seven-point route, without validating the owner's arrival.
- 205051 is the native summon controller: aura 428 triggers 192467, summoning Oro 97552 with properties 3054. Removing the controller removes its summon.
- 192936 summons grasp 97781 using vehicle category 4/properties 3035. The production summon effect boards the caster in vehicle 4404, seat 15989. Successful summoning alone does not prove boarding succeeded.
- Click spell 193017 is **effect 165, 500% max-health damage**, not a despawn effect. The spell-click handler has no owner/seat-validated TALKTO credit. Any previous credit depended on the generic kill-credit path (which also accepts TALKTO objectives), rather than proof that this player actually escaped this vehicle.

The quest narrative is corroborated by [Wowhead](https://www.wowhead.com/quest=39027/dargrul-and-the-hammer) and [Warcraft Wiki](https://warcraft.wiki.gg/wiki/Dargrul_and_the_Hammer). The [2015 alpha walkthrough](https://warcraft.blizzplanet.com/dargrul-and-the-hammer-highmountain-quests-alliance/) is supporting narrative evidence only, not an authoritative source for 7.3.5 timings or IDs.

## Repair

- A personal Oro uses all seven existing route points, waits within 15 yards of the owner between legs, and credits following only after actual final arrival. A failed movement times out without credit. Reconnect resumes from the nearest existing cave waypoint; no companion is spawned at the outdoor quest giver and sent directly through the cave wall.
- After following, entering the chamber starts personal Dargrul and Ularogg instances. They retain the existing jumps, hammer 193100, dialogue, departure path and timed despawn. Other players no longer consume the same one-shot encounter. The grasp follows the existing five-second hammer cue; confrontation is credited only by the owner's seat-zero boarding callback.
- Each explicit objective credit refreshes quest-dependent phases and area spells through the existing quest-status API, including the transition to the petrified-warbrave phase. Merely changing a saved objective counter does not perform this refresh.
- Escape retains native click spell 193017 but replaces its damage effect with explicit owner/quest/seat/lifetime validation, successful vehicle exit, quest-specific TALKTO credit, and destruction of that hand. A nearby click or another player's hand cannot grant progress or be destroyed by this spell.
- Failed boarding, manual vehicle exit, death, leaving the chamber, logout, map change and abandonment allow recovery without granting missing objectives. The existing core quest lifetime token cancels deferred actions from an abandoned/reaccepted quest; scene GUIDs also prevent an old scene's timer from unlocking a new scene.
- The cave-exit event requires all three preceding objectives. Scene 6409 and its distinct quest 41764 are preserved, with the correct event-object condition source.
- Login repairs premature cave-exit credit on active quests that lack earlier objectives, including incorrectly complete but unsubmitted quests. Rewarded quests are untouched.
- Native hidden flags are restored. **Deployed storage slots are deliberately retained**: follow=0, confront=1, grasp=2, exit=3. TDB735 uses confront=0/follow=1, but changing those slots in a world-only update would reinterpret existing character progress. The server publishes its slots in the quest query; slot order is not used as a substitute for event prerequisites.
- Quest giver 97553, turn-in 93805, existing phase definitions 5040/5041/5042, the 39043 → 39027 → 38909 → 38912 chain, and static NPC placements are retained. No boost, Broken Shore, pet, camera, or global criteria handlers changed.

## Verification

- `tools/tests/Test-HighmountainDargrul.ps1`: compiles and executes extracted production escort, player, grasp and spell handlers, plus the production delayed-quest helper/status type. Covers two players, arrival/boarding gates, unrelated clicks, failed release, retries, death, reconnect, same-tick abandonment/reacceptance and old save reconciliation. Test doubles represent the world/vehicle APIs; this is not an in-game playthrough.
- `tools/tests/Test-HighmountainDargrulNative.py`: native objective records and flags, controller/escort effects, summon properties, grasp vehicle/seat, hammer and escape effect, with source hashes.
- `tools/tests/Test-HighmountainDargrulData.ps1`: applies the migration twice to temporary copies of the affected tables; validates bindings, save slots, all exit conditions, preserved companion scene and actor timelines, prerequisites and unrelated data. `-Live` performs read-only validation after deployment.
- Read-only probe linked against the actual Release MMAP/Detour libraries: all six consecutive route segments have connected navigation polygons, with path lengths 1, 3, 3, 4, 2, 1 polygons. Uses map 1220, tile 25/23 and the existing path 97552 coordinates.
- Release script library compilation succeeded. Final runtime build and startup are checked during deployment from `build-extractors/bin/Release`.

## Remaining client checks

An in-game 7.3.5 playthrough is still required. Check the visible Oro follow, personal actor presentation alongside existing static spawns, hammer and petrified warbraves, grasp boarding/click/exit animation, pet frame, cave-exit credit, Ebonhorn turn-in and availability of 38909. Repeat with two players and after abandonment/reconnect. Native IDs and server-side tests do not verify animation, camera, or client presentation. Dialogue timings are retained from this server's existing SmartAI data, not claimed as a retail sniff reconstruction.

Next separate quest: **38909 — Get to High Ground**, then 38912. This repair does not claim those follow-up quests have been playtested.
