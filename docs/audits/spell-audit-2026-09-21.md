# Full spell audit — 2026-09-21

## Scope and reference version

This audit targets the local Legion 7.3.5 client data, build 26972. It compares
the spell data actually loaded by the core with source and world-database
references. The audit covers:

- `Spell.db2`, `SpellEffect.db2`, `SpellAuraOptions.db2`, `ItemEffect.db2`,
  `ItemSet.db2` and `ItemSetSpell.db2`;
- the SQL hotfix overlays for spells, effects and item effects;
- spell-related fields in `legion_world`;
- spell constants and direct spell API calls in the C++ source tree;
- all Antorus Tier 21 item sets and their 2-piece/4-piece mechanics;
- a clean Release build and full worldserver startup validation.

The client data is the primary numeric source of truth. External checks used
the official [7.3.5 patch notes](https://worldofwarcraft.blizzard.com/en-us/news/21365423),
[Wowhead's Tier 21 reference](https://www.wowhead.com/guide/tier-21-armor-sets-antorus-the-burning-throne-transmog-5326),
the [LegionCore 7.3.5 upstream repository](https://github.com/The-Legion-Preservation-Project/LegionCore-7.3.5),
and mechanic-specific historical reports where client data alone did not
describe server-side behaviour.

## Final result

The active repository-level spell references now agree with the effective
7.3.5 DB2/hotfix catalogue. The remaining numeric mismatches are either inert
records inherited from Blizzard's 26972 client files, an SQL item table that
this core does not load, or a disabled custom event whose private spells were
never part of the retail client.

| Area | Final result |
| --- | ---: |
| Raw `Spell.db2` records | 179,382 |
| Effective DB2 + hotfix spell IDs | 179,641 |
| Raw `SpellEffect.db2` records | 707,841 |
| Invalid effect indexes / effect types / aura types | 0 / 0 / 0 |
| Duplicate raw effect slots | 0 |
| Duplicate effective hotfix effect slots | 0 |
| Missing `ItemSetSpell` spell IDs | 0 |
| Missing `SpellAuraOptions` owner spell IDs | 0 |
| Initial world DB mismatches | 1,299 rows / 882 IDs |
| Remaining active world DB mismatches | 0 |
| Remaining active C++ spell mismatches | 0 |
| Tier 21 bonus rows without a spell/effect | 0 |
| Tier 21 trigger spells without an effect | 0 |

## DB2 and hotfix integrity

The effective catalogue is the union of the 179,382 client spell records and
260 hotfix spell rows (259 hotfix-only IDs). The effective spell-effect overlay
contains 312 SQL rows and has no duplicate `(SpellID, DifficultyID,
EffectIndex)` slots after the deterministic repair for custom spell 305010.

The 26972 client files themselves contain several dangling references:

| Client structure | Rows | Unique missing IDs |
| --- | ---: | ---: |
| `SpellEffect` owner spell | 9 | 7 |
| `SpellEffect.EffectTriggerSpell` | 2,283 | 1,157 |
| `ItemEffect.SpellID` | 775 | 495 |

The SQL hotfix overlay does not replace these records. They include obsolete,
test and retired client assets and are inert because `SpellMgr` creates a
`SpellInfo` only for an effective `Spell.db2`/hotfix spell. A numeric dangling
reference does not identify a canonical replacement, so inventing server-side
spell definitions would make the data less faithful to build 26972.

## World database repairs

The initial scan checked 216,677 references representing 54,732 spell IDs.
Every mismatch in a runtime-loaded world table was reviewed and either repaired
against build 26972 or removed when the owning content was obsolete. The work
was committed and pushed in small, independently reviewable changes.

The repaired areas include:

- stale creature actions and creature spell slots;
- 576 orphan spell-loot definitions;
- area-trigger templates, data and polygons, including aura-created triggers;
- Demonic Gateway's reuse-cooldown condition;
- retired aura-dummy, proc-check and trigger branches;
- obsolete quest reward/source spells and their dead content;
- stale spell-script bindings and obsolete quest/instance helpers.

After the repairs, every initially reported field group is at zero except the
legacy SQL `item_template` fields: 312 references / 248 IDs. This is not runtime
item data. `ObjectMgr::LoadItemTemplates()` builds items from `ItemSparse.db2`
and appends effects from `ItemEffect.db2`; it never selects spell fields from
`item_template`. Of the 312 legacy fields, 255 have no corresponding DB2 item
effect and 57 mirror the client file's own obsolete/test dangling effects.

## Source audit

The high-confidence scan initially produced 210 IDs requiring review. After the
mechanic-by-mechanic repairs, an exact rescan of current declarations leaves 58
static candidates:

- 43 private 305xxx IDs in the disabled Temple section of
  `World/custom_events.cpp`; all related registrations are commented out;
- 14 values that are timers, quest/creature IDs, achievement keys, deliberately
  custom non-DBC OutdoorPvP dispatch values, or a deprecated commented cast;
- spell 99771, which is supplied by the hotfix spell table.

None of these is an active missing retail spell. The disabled Temple code was
not assigned invented retail IDs and was not enabled.

## Antorus Tier 21 verification

The maximum retail class tier for Legion is present as item-set IDs 1319–1330:
12 class sets, six items per set. `ItemSetSpell.db2` contains 72 bonus rows,
covering all 36 specializations with both a 2-piece and a 4-piece threshold.

All 72 bonus spell records have effects. Their 27 directly triggered spell
records also exist and have effects. Native modifier/proc-trigger bonuses were
checked against their DB2 effect and proc data; dummy mechanics were checked
for a concrete C++ consumer and, where necessary, a validated
`spell_script_names` binding.

The audit restored the missing Paladin Protection and Priest Shadow bindings,
the class-specific T21 bindings for the other scripted bonuses, and one missing
implementation: Warlock Affliction 2P (251847). Its DB2 proc supplies the
retail 8% chance, while the new handler extends each active caster-owned
Unstable Affliction on the Agony target by the DB2 amount of 2,000 ms. This
matches the 7.3.5 tooltip and the documented retail tick behaviour in the
[resolved Legion report](https://tracker.legionbugs.com/view.php?id=12200).

The result establishes that every T21 set grants both bonuses and that each
bonus has a native effect path or a registered server implementation. Full
combat-output parity still requires gameplay tests with equipped characters;
static data and startup validation cannot replace encounter-level simulation.

## Build and runtime validation

The final Release `worldserver` build completed successfully at revision
`b42242f`. On a clean startup, the database updater applied updates 267–280 in
order, including the conflict-safe area-trigger migration.

Runtime results:

- 3,593 spell script names loaded;
- 7,401 C++ scripts loaded;
- 3,741 script bindings validated;
- world initialization completed in 14 seconds and reached `ready`;
- `DBErrors.log` remained empty;
- the server shut down cleanly.

The spell-audit changes span `66519c4` through `b42242f`; each gameplay or data
repair was committed and pushed separately.
