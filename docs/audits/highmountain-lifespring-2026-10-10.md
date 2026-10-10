# Lifespring companion and ground crystals — 2026-10-10

Follow-up to `highmountain-cavern-2026-10-10.md`. Migration: `2026_10_10_08_highmountain_lifespring.sql`; production handler: `highmountain_lifespring.cpp`.

## Companion

The installed 7.3.5 DB2 contains summon spell 190370, effect 278877, creature 96038 and SummonProperties 3679. Creature 96038 is named **Rivermane Shaman** and uses display 65483. Stationary Jale is a different entry, 96520/display 63712. This repair uses the native companion identity; it does not rename the shaman or claim to have proved that it must use Jale's model. Native records and SHA-256 hashes are preserved in `highmountain-lifespring-native-2026-10-10.json`.

The old `spell_area` row required High Water 39498 to be complete/rewarded before summoning (end-status mask 66), and depended only on 39488. The replacement follows either parallel quest, including either reward order and the transition to Crystal Fury. It requires rewarded 39661 and a living player in area 7786/map 1220. Accepting High Water, leaving the area/map, dying, entering a vehicle/flight or logging out removes the owned summon. Abandoning all cavern quests before either parallel reward also removes it; a rewarded prerequisite retains the companion so the player can obtain/reaccept the next quest. Abandoning High Water allows it to return in the cave.

Recovery is throttled to one second, removes duplicate/dead owned summons and restores a missing companion. The NPC follows three yards behind, is visible to its owner, stays passive and is recreated beside the player after a phase mismatch or separation beyond sixty yards. It offers all four relevant quests, including the previously absent 39488 relation. Existing turn-in relations and dialogue are retained. The stationary 96038 spawn still delegates to SmartAI. No quest credit or reward is granted automatically.

SummonProperties flags include 512: this core creates a Guardian. However, Control=ALLY, Title=0 and Slot=0 mean it is neither a primary pet nor a controllable guardian. Reviewed `Map::SummonCreature`, `Guardian` construction, `Unit::SetMinion`, `Spell::SummonGuardian` and `Guardian::InitStatsForLevel`: the companion does not replace the primary pet, clear its client summon field, or erase the quest-giver flags. This is code/data inspection, not a client pet-portrait test. Native target 142 falls back to the caster destination; the database has no overriding target position/condition for 190370.

## Ground crystals: reconstructed placement, not retail coordinates

No authoritative coordinates were found in the local TDB735 archive, the original LegionCore database, the current TDB archive or installed GameObjects.db2. The twelve new locations are explicitly a functional reconstruction. They were chosen near existing cave enemies, placed 0.08 yards above the extracted VMAP floor and checked against the installed MMAP. Candidates on disconnected ledges were rejected.

All twelve final nodes have a nearby navigation polygon and a complete navigation path from the first crystal inside the cave. This does **not** prove a navigation path from stationary Jale: the entrance NPC's nearest polygon is isolated in these extracted maps. Nor does it prove client model appearance. The companion's distance recovery accommodates a lost follow path; the client cave-entry/follow sequence still needs a playthrough.

The migration uses native object 243639/display 28054, existing quest item 128393, loot table 243639, native lock 1691 and template quest-item visibility. The DB2 lock is ordinary Open (skill type 5, required skill zero). Each node yields one quest-only crystal while needed; ten complete 39488. Twelve nodes respawn after 120 seconds. Existing enemy drops remain available. Auto-increment assigns GUIDs, and applying the migration again does not duplicate nearby nodes or replace existing placements. The native crystal flags and models are preserved.

## Validation

- `Test-HighmountainLifespringNative.py`: installed DB2 summon/lock records and evidence hashes.
- `Test-HighmountainLifespring.ps1`: compiles the actual production namespace, spell gate, NPC AI and PlayerScript with a test adapter. Exercises both initial quest orders, complete/reward gaps, abandonment/reacceptance, duplicate cleanup, two-player ownership, death/resurrection, logout/map cleanup, follow restart, phase/distance recovery and the stationary NPC path.
- `Test-HighmountainLifespringData.ps1`: migration applied twice to temporary copies; checks twelve unique nodes, native loot/quest links, spell/script binding, preservation of existing phase/stationary spawn and unrelated data. Also verifies a pre-existing custom NPC script is not overwritten. `-Live` checks the installed migration.
- `Test-HighmountainLifespringGeometry.ps1`: links the actual Release collision/navigation libraries, reads migration coordinates and validates floor height, nearby polygons and complete paths against the installed data. Requires an existing Release build and extracted VMAP/MMAP files.
- Existing cavern production/data regression checks remain applicable to the prerequisite pair, alternate kill credit and Gelmogg's second phase.

Release build and server startup must use `build-extractors/bin/Release`. The boost/ship/Broken Shore, camera, pet and global quest-criteria code are unchanged.

Client checks still needed: enter the cave with either quest first; inspect the companion's appearance and quest menu; gather ten ground crystals and check sparkle, click/loot, disappearance/respawn and loss of quest eligibility; turn in both quests in either order; complete Gelmogg and accept/abandon/reaccept High Water; leave/re-enter, die/resurrect and relog; confirm two players do not share companions and the permanent pet portrait stays intact. Server tests do not mark this playthrough verified.
