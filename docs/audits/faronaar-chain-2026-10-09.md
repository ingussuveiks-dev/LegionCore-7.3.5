# Faronaar continuation: From Within to Dark Revelations

Scope: the normal **36920 → parallel 37656/37450 → 37449** continuation after
The Scythe of Souls. The local quest conditions already require both parallel
quests before Dark Revelations; that relationship is preserved. From Within is a
delivery quest: the 7.3.5 archive has no objectives for its three variants, unlike
the previously broken Scythe of Souls. This pass does not invent class-variant
starters or convert the questline's display order into linear prerequisites.

## Confirmed repairs

- **Questgiver recovery:** the original 178860 summon produces personal NPC
  90474 after From Within. The old `spell_area` cast was a one-time transition
  with no reliable reconnect recovery. The player script supplies the spell's
  destination explicitly, restores a missing companion, retains existing quest
  menus, follows the player and removes it outside the eligible phase. Copies
  have a finite lifetime; the original questgiver ID and spell are unchanged.
- **Fel Machinations (37656):** existing harvester objects are matched to a live
  nearby captive 90487. The actual user's interaction supplies one credit and
  triggers the existing release dialogue. The old timed list credited every
  player within ten yards, including non-users. That shared credit is removed.
  Six distinct harvester spawns are required; repeated use cannot double count.
- **Saving Stellagosa (37450):** native lock 2358 requires item 120359. Discovery,
  key possession, range, line of sight and one of the three original lock spawns
  are checked. Each lock counts once per character. The native first-chain credit
  now follows a real unlock, not a shared NPC timer. The key's objective Flags2
  is restored to the 7.3.5 archive value 1. Its 100% quest-required Lykill loot is
  already present and remains unchanged.
- **Personal chains:** the exact existing positions of Stellagosa 90546 and the
  three beam actors 90578 are retained. Their static copies are replaced with
  personal actors using native chain effect 65612. Unlocking removes only that
  player's corresponding beam; all three releases let her rise and depart.
  The old global reset loop could despawn/rechain the dragon for unrelated players.
- **Dark Revelations (37449):** Nightglaive's existing introduction, combat and
  actual kill-credit path remain. After the kill, a personal native Stellagosa
  vehicle 90982/4103 is available near the player at the overlook. It is optional:
  native summon 178923 forces boarding, so the script creates its native NPC with
  the existing 46598 spellclick instead. The seven original SmartAI waypoints
  begin on passenger boarding, not creation. Boarding grants only the optional
  ride objective. Leaving early removes the ride. The end of the route despawns
  the vehicle using normal vehicle teardown; shared mover code is unchanged.
- **Mandatory return:** 112175 requires a real Nightglaive kill and actual arrival
  near the original Illidari leader, with line of sight and outside a vehicle or
  flight. Walking back works too. An empty departing vehicle or absent summoner
  can no longer complete the return objective remotely.

The small character ledger records only `(character, quest, spawn, ordinal)` for
the two counted interactions. Uses survive logout. On abandon/reaccept or a saved
progress rollback, ordinals beyond current quest progress are removed; other
characters and quests remain independent. Existing partial progress predating
this repair is retained, but its unknown historical object identities are not
fabricated. Death/area changes clear personal actors; old-map copies have finite
lifetimes and are cleaned before recreating them on a quick return.

## Evidence and validation

The 2018-02-19 TDB 7.3.5 archive (objective build 25549) confirms the eight objective
records, including the **optional** flight. Extracted 26972 DB2 confirms the key
lock, summon identities/properties, native beam effect, vehicle/seats and questline.
Hashes and records are in `faronaar-native-2026-10-09.json`.
[Saving Stellagosa](https://www.wowhead.com/quest=37450/saving-stellagosa)
corroborates the key and three-chain interaction; modern reward/level data is not
imported. No replacement quest ID, target ID or objective count was invented.

- `Test-FaronaarChain.ps1` compiles the complete production C++ handlers against
  a small world/database boundary. It covers two players' separate actors and
  credits, duplicate/relogged use, rollback/reaccept, key/range/LOS requirements,
  failed summons, companion recovery, optional boarding and mandatory return.
- `Test-FaronaarData.ps1` applies the production world migration twice in isolated
  temporary tables and checks native loot, prerequisites, route and custom-data
  preservation. The character schema is exercised only as a temporary table,
  including per-character/per-quest uniqueness and isolated reset.
- `Test-FaronaarNative.py` checks the original 7.3.5 identities and objective flags.
- Release compilation and live startup are checked separately during deployment.

Full client playthrough, chain/flight visuals, passenger camera and pet frame still
need confirmation. Handler tests and successful startup do not verify those. This
repair does not reconstruct every original companion line or cinematic. The
custom level-100 boost, its ship/bird scenario, shared scenario criteria, pet lookup
and warlock handlers are unchanged. Azurewing Repose and the remaining Azsuna
chains are outside this pass.
