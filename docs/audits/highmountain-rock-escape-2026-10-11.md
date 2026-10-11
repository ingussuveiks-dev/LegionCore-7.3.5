# Highmountain: Rock Troll in a Hard Place (39381), 2026-10-11

## Result and scope

The cave escape now validates the actual guide owner, permits retries after native talk credit, preserves the guide until arrival, and recovers a missing guide for completed-but-unrewarded saves still inside the cave. The three refuge quests are offered in parallel with a combined reward gate for Blood Debt. No boost, ship, global criteria, pet lookup or mover-control code changes.

## Evidence and defects

TDB735 identifies objective 280082 as one personal monster credit 96286, and 95421 as the ender. The installed DB2 spell 190967 gives that credit immediately, then applies a 1750 ms stun, a periodic trigger and child spell 190983. The original SmartAI subsequently teleported the summoner after 3000 ms and unconditionally despawned the guide. Its gossip condition allowed only incomplete 39381; the preceding batch's recovery also excluded completed 39381. Thus a player disconnected between credit and teleport could lose the escape option and guide. A forged interaction could also select a different player's guide, because the old list cast on the invoker but teleported the summoner.

The native credit timing is intentional: the objective is speaking to Navarrogg, not an invented arrival objective. [Blizzplanet's firsthand Legion quest recording](https://warcraft.blizzplanet.com/rock-troll-in-a-hard-place-highmountain-quests/) describes selecting the escape dialogue, Stoneshift, credit, then completion at the refuge. [Wowhead 39381](https://www.wowhead.com/quest=39381/rock-troll-in-a-hard-place) corroborates the objective. Modern rewards and levels are not copied.

The old destination is retained: map 1220, 5435.1, 4497.69, 642.4, orientation 3.467. Existing refuge Navarrogg 95421 (GUID 340571) is at 5429.69, 4491.97, 641.583. Actual installed NAV_GROUND data connects them through one polygon; the destination is not an unverified invented coordinate.

## Implementation

- A SmartAI subclass preserves the guide's existing personal visibility, follow and conversation 212762. Repeated spell_area summons are deduplicated before those actions run.
- Only menu 18673/option 0 starts the new path. The guide must belong to this player; both must be alive and in the same phase, with visibility, line of sight and six-yard proximity. The player must be inside cave area 7807 with incomplete or completed/unrewarded 39381, outside vehicles/flights/another teleport.
- Native 190967 still grants the talk objective and its existing presentation. Repeated clicks cannot schedule multiple teleports. The three-second delay is retained.
- Quest-scoped delayed events invalidate on abandon/reaccept. Before transport, ownership, quest, location and interaction conditions are checked again. Death, moving away, changing maps, failed TeleportTo, or a missing source guide cannot teleport an unrelated/stale player.
- TeleportTo's true return is not treated as arrival: it can defer movement. The player is checked every 100 ms for up to five seconds; only observed arrival near the destination with teleport no longer pending despawns the original guide. Failure/timeout keeps retry possible. Cleanup uses the original GUID, never a nearest-guide search.
- The gossip condition now allows incomplete OR complete. Blood Rite recovery also permits complete 39381 while still in the cave; rewarded 39381 remains excluded. No quest progress is reset to manufacture a retry.
- TDB735 gives all three refuge quests (39391, 39425, 39588) RewardNextQuest 39426. The live DB instead serialized them. Their prerequisites now point to rewarded 39381, their next link to 39426, and negative exclusive group -39391 makes Blood Debt require all three rewards. Their objective/item/kill implementations are unchanged and are subsequent audit work. Bonus quest 40243 is unchanged.

## Validation

- `Test-HighmountainRockEscape.ps1` compiles extracted production handlers: native credit timing, wrong owner/menu/phase/LOS/area, double clicks/summons, simultaneous players, failed/deferred/timeout teleport, actual arrival, death, map/range changes and abandon/reaccept cancellation.
- Updated `Test-HighmountainBloodRite.ps1` verifies recovery for completed 39381 and exclusion after reward. Its data test now expects the preserved SmartAI behavior through the new subclass rather than the removed unsafe timed list.
- `Test-HighmountainRockEscapeNative.py` verifies the archived objective/ender/next links and native effects/duration; adjacent JSON stores evidence and hashes.
- `Test-HighmountainRockEscapeData.ps1` applies the migration twice to temporary tables and checks bindings, preserved summon/conversation, native objective, both menu branches, actual refuge NPC, branch gates and unrelated records. `-Live` verifies deployed data.
- `Test-HighmountainBloodtotem.ps1` compiles the real core prerequisite handler and now checks all eight reward subsets for both the earlier Bloodtotem group and the new refuge group. Existing inventory/login/reward regressions remain intact.
- `Test-HighmountainRockEscapeGeometry.ps1` links the actual Release navigation library and checks the installed ground data from landing point to turn-in NPC.
- Final Release/startup validation uses `build-extractors/bin/Release`, with database and executable backups and protected runtime hashes.

## Follow-up: absent trigger handled (2026-10-11)

Migration 08 and `aura_rock_escape_transport` now handle effect 2 directly. The native 1200 ms tick suppresses the default attempted cast of absent 190982 and dispatches only a validated, pending interaction on this player's guide. It calls the same checked transport/arrival logic. The three-second callback remains a fallback for a missing/dispelled aura; per-attempt lifetime and departed state prevent a second teleport or an old retry timer acting on a new interaction. Credit, stun, 190983 and client DB2 records remain unchanged. This supplies the required server behavior without claiming that a guessed spell is the original 190982. Tests compile and exercise the real periodic handler, including unauthorized casts, duplicate ticks, failed dispatch, stale fallback and abandonment.

## Original limitation and remaining client check

The native periodic trigger 190982 is absent from the installed Spell/SpellEffect/SpellMisc data and has no hotfix row here; searches did not establish an authoritative replacement. This batch does not invent an ID or alter those client records. The missing-trigger execution is now explicitly handled by the follow-up above; native 190967/190983 presentation is retained. A 7.3.5 client retest must still confirm the stone-fist/stun presentation, pet portrait, arrival, turn-in and all three offers. Compilation and scripted tests are not proof of client visuals or a complete retail playthrough.
