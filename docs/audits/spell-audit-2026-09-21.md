# Full spell audit — 2026-09-21

## Scope

This audit compares the LegionCore spell data used by the server against the
local 7.3.5 client DB2 files and the SQL hotfix overlay. It covers:

- `Spell.db2` and `SpellEffect.db2` structural integrity;
- the effective spell catalogue after applying `legion_hotfixes.spell`;
- `legion_hotfixes.spell_effect` ownership, bounds and duplicate slots;
- spell IDs referenced by spell-related columns in `legion_world`;
- spell constants and direct spell API calls in the C++ source tree;
- database script-name bindings and a clean worldserver startup validation.

Static source findings are candidates for review, not proof that a code path is
reachable. Old-expansion, disabled and commented custom content is still present
in the source tree and is intentionally reported rather than silently ignored.

## Result

Not every spell reference in the repository exists in the effective 7.3.5
DB2/hotfix catalogue. The active spell loader and script registry are healthy,
but the repository contains a substantial legacy-data backlog.

| Area | Result |
| --- | ---: |
| Raw `Spell.db2` records | 179,382 |
| Raw `SpellEffect.db2` records | 707,841 |
| Effective DB2 + hotfix spell IDs | 179,641 |
| Invalid raw effect indexes/types/aura types | 0 / 0 / 0 |
| Raw duplicate effect slots | 0 |
| Raw effects whose owner spell is absent | 9 |
| Raw trigger references whose target spell is absent after hotfix union | 2,283 rows / 1,157 IDs |
| Hotfix spell rows | 260 |
| Hotfix-only spell IDs | 259 |
| Hotfix effect owner/trigger/bounds errors | 0 / 0 / 0 |
| Hotfix duplicate effect slots after repair | 0 |
| DB spell script names with no source occurrence | 0 of 2,769 |
| Likely source spell declarations checked | 14,492 declarations / 13,912 IDs |
| Referenced source IDs absent from the effective catalogue | 187 declarations / 182 IDs |
| Missing direct spell-call literals | 31 IDs |
| Combined high-confidence source review set | 210 IDs |
| World DB spell references checked | 216,677 rows / 54,732 IDs |
| World DB references absent from the effective catalogue | 1,299 rows / 882 IDs |

The world DB result excludes negative nested group identifiers in `spell_group`
and `spell_enchant_proc_data.entry`, which is an enchantment identifier rather
than a spell identifier.

## Safe repair made

`legion_hotfixes.spell_effect` contained two rows for spell 305010, difficulty
0, effect index 0. Loader order already made the newer row (`ID=410010`, base
points 1) win. Update
`2026_09_21_001_deduplicate_spell_305010_effect.sql` removes only the obsolete
row (`ID=16`, base points 0), preserving effective runtime behaviour while
making the data deterministic.

Post-update verification:

- 312 hotfix effect rows;
- one row for spell 305010: `ID=410010`, effect index 0, base points 1;
- zero duplicate `(SpellID, DifficultyID, EffectIndex)` groups.

Commit: `66519c4 Deduplicate custom spell 305010 effect`.

## Remaining findings

### Client DB2 inheritance

Nine `SpellEffect.db2` rows have no owning `Spell.db2` or hotfix spell record:

- spell 182749, effect row 264503;
- spell 183093, effect row 265051;
- spell 183531, effect row 265838;
- spell 184054, effect row 266671;
- spell 227478, effect row 359762;
- spell 237658, effect rows 360019 and 360021;
- spell 74063, effect rows 469868 and 469869.

These are inert client-data remnants. The server cannot safely invent missing
spell definitions for them. The same rule applies to the 1,157 absent trigger
targets: a missing numeric target does not identify the intended replacement.

### C++ source backlog

The 210-ID high-confidence review set is concentrated in legacy or dormant
content. The largest groups are:

- `World/custom_events.cpp`: 38 IDs, mostly disabled custom Temple event code;
- `Northrend/Naxxramas/boss_kelthuzad.cpp`: 19 IDs;
- `Kalimdor/ThroneOfTheFourWinds/boss_alakir.cpp`: 18 IDs;
- `Scenario/ProvingGrounds/proving_grounds.h`: 9 IDs.

Some core call sites also retain removed pre-Legion IDs, for example Battle
Stance 2457. Removing or remapping these without validating the surrounding
mechanic would change gameplay and is not a safe mechanical repair.

### World database backlog

Absent spell references by table/field group:

| Table | Rows | Unique IDs |
| --- | ---: | ---: |
| `spell_loot_template` | 889 | 576 |
| `item_template` | 312 | 248 |
| `areatrigger_template` | 37 | 35 |
| `areatrigger_polygon` | 28 | 6 |
| `quest_template` | 9 | 6 |
| `creature_action` | 6 | 4 |
| `spell_trigger` | 5 | 5 |
| `areatrigger_data` | 4 | 4 |
| `spell_aura_dummy` | 4 | 2 |
| `spell_proc_check` | 2 | 2 |
| `areatrigger_actions` | 1 | 1 |
| `creature_template_spell` | 1 | 1 |
| `quest_template_addon` | 1 | 1 |

Most rows belong to imported legacy loot and item data. Bulk deletion would
hide the mismatch but could remove intended content; bulk substitution would be
worse because DB2 absence provides no canonical destination ID.

## Runtime validation

A clean Release worldserver startup after applying the repair reported:

- 3,597 spell script names loaded;
- 7,406 C++ scripts loaded;
- 3,747 script bindings validated;
- world initialization completed and the server reached ready state;
- `DBErrors.log` remained empty;
- the server was shut down cleanly.

## Recommended repair order

1. Review missing IDs in active core code and active Legion scripts, mechanic by
   mechanic, using the intended 7.3.5 spell chain as the source of truth.
2. Validate active area-trigger, quest, creature and spell helper rows in the
   world database and repair each content unit separately.
3. Audit legacy expansion scripts only when that expansion's content is being
   brought into the supported runtime path.
4. Remove disabled custom-event remnants only as an explicit content-retirement
   decision, not as part of numeric spell cleanup.

No mass replacement or deletion is recommended. Each remaining repair needs an
authoritative intended spell or an explicit decision that the content is dead.
