# Talent, equipment and aura mechanics audit — 7.3.5.26972

Follow-up: [cast resources and orphan-link resolution](cast-resources-2026-10-08.md)
documents later repairs and evidence-based retirement of the four native links
listed below. This report and its JSON retain the original audit checkpoint.

This follow-up checks the three next priorities from the profession lifecycle
audit: specialization/talent transitions, item/enchant requirements, and aura
stack/charge behavior. Supporting DB2 reference checks also exposed one broken
achievement hotfix. Four areas were repaired; live client acceptance is pending.

## Repairs

### Removed PvP talents were resurrected by always-on configuration

`TogglePvpTalents` evaluated `(enable && selected) || alwaysOn`. Therefore a
`PLAYERSPELL_REMOVED` entry, which remains in the talent map until persistence,
could be learned again when `CONFIG_PLAYER_ALLOW_PVP_TALENTS_ALL_THE_TIME` was
enabled. Its override was also restored. The check is now
`selected && (enable || alwaysOn)`. The setting still keeps selected talents
available outside PvP; it no longer changes which talents are selected.

The extracted production test covers both configuration values, enable/disable,
removed entries, inactive talent groups, missing DB2 entries, level and dungeon
restrictions. It also exercises production specialization learn/remove and both
override indexes, shared spells, mastery aura cleanup and level requirements.
Full `ActivateTalentGroup`, database persistence and client action bars are not
simulated by these fixtures.

### Complete matching armor failed the specialization requirement

`HasItemFitToSpellRequirements` checked the eight required armor slots but fell
through to the default/false result after all eight passed. It now returns true
after a successful full-set check. The same path now respects `ignoreItem`, so
checking the set while a required item is being removed cannot count that item.
Ordinary armor checks break cleanly instead of logging an unsupported class.

Tests combine the production Player requirement function with production
`Item::IsFitToSpellRequirements`: complete set, wrong subclass, missing slot,
ignored/removed piece, ordinary one-item requirements, enchant inventory masks
and the existing vellum exception. Actual stat application and passive aura
recalculation remain client/runtime acceptance checks.

### Stack/charge redirection lost removal results and reason

`ModCharges` sometimes delegates to `ModStackAmount`, and `ModStackAmount`
delegates to `ModCharges` for SQL `modcharges`. Both paths discarded the boolean
that reports final aura removal. The former also discarded the removal mode.
Both now return the delegated result, and charge-to-stack conversion forwards
the removal mode. Tests cover final-stack/final-charge removal, enemy-spell
removal mode, maximum stack/charge limits and no duration refresh on stack loss.
These compile the production functions with mocked low-level aura operations;
they do not simulate every AuraScript callback or proc probability distribution.

The eight local SQL `modcharges` rows were reviewed against aura stack limits;
none has a native multi-stack limit requiring a new recursion workaround. Proc
scripts and custom C++ attributes are not exhaustively modeled by the DB2 audit.

### Exodar achievement hotfix pointed to an unrelated quest

`criteria_tree` 7898 described Exodar reputation but used Criteria 11321
(type 27, quest 13724), and Parent 5332 did not exist in CriteriaTree. The native
26972 row supplies Criteria 5332 (type 46, faction 930), Parent 7896 and OrderIndex
1. Tree 7896 belongs to Achievement 2761. The migration restores those three
fields and sets VerifiedBuild=26972 only for the exact broken old mapping.

`2026_10_08_002_restore_exodar_criteria_tree.sql` was applied to the local hotfix
database. It replaces two ambiguous old notifications with one fresh notification
for CriteriaTree table hash 1255424668, read from the WDC1 header. Before/after
exports were retained privately. A second application produced identical row
and notification exports. No saved character achievement/progress was edited.
This fixes achievement data; scenario handling and the boost sequence were not
changed or claimed to be tested in game.

## DB2 and SQL reference evidence

`audit_spell_mechanics.py` reads WDC1 using the repository's expected layout
hashes and field definitions, including signed values, arrays, copy rows and
relationships. It overlays exported SQL data and rejects missing or stale
nonempty overlays. Native and SQL hashes are in the adjacent JSON.

| Primary data set | Records |
| --- | ---: |
| ChrSpecialization (including non-player entries) | 42 |
| Talent / PvpTalent | 617 / 387 |
| SpecializationSpells | 899 |
| SpellEquippedItems | 2,642 |
| SpellAuraOptions | 13,054 |
| SpellProcsPerMinute / modifiers | 197 / 738 |
| SpellReagentsCurrency / SpellTotems | 765 / 3,178 |
| Criteria / CriteriaTree | 26,680 / 49,730 |
| Scenario / ScenarioStep | 441 / 1,638 |

Checks cover talent spell/override/spec references, class/spec compatibility,
talent grid limits, mastery references, aura-to-PPM and PPM-modifier references,
currency and totem-category references, criteria parents and scenario tree links.
Cooldown (17,183), power (4,614) and skill/race/class (150) records were also read;
their individual balance values, requirements and every SQL/C++ override were
not comprehensively validated. Reading a record does not verify its behavior.

After the confirmed hotfix repair, four missing references remain **in native
client data**, not introduced by the local SQL overlays:

- SpecializationSpells 4946, shadow specialization 258, references absent spell
  194248. `LearnSpecializationSpells` already skips missing SpellInfo records.
- ScenarioStep 1947 for scenario 908 references absent tree 31016.
- ScenarioSteps 2233 and 2234 reference absent scenario 1045.
  The `ScenarioMgr` constructor skips steps whose scenario is missing.

These are unresolved data references, not proven live gameplay defects. No
replacement IDs were established, and no guessed replacements or wholesale
deletion of native rows was introduced. The audit reports these findings;
successful execution is not a claim that every reference is valid.

## Validation and runtime

- The new `Test-SpellMechanics.ps1` compiled old production handlers and failed
  on PvP resurrection, full armor acceptance and both removal redirections.
  It passes after the fixes.
- GlyphLifecycle, SkillLifecycle, ProfessionRegression and WarlockRegression
  suites also pass. Existing numeric-conversion/harness warnings remain.
- Release `worldserver` and `bnetserver` built successfully in the canonical
  `build-extractors/bin/Release` runtime. Configs, certificates, DLLs and extracted
  data were preserved.
- `Test-WorldShutdown.ps1 -ProbeConnections`: ready reached, both world ports
  probed, exit 0, no new crash dump. DBErrors.log retains only the previously
  documented unrelated waypoint-script 347 warning for this startup.

No live talent changes, equipment stat changes, proc timing, achievement credit,
client visuals or relog acceptance were performed while the user was away.

## Reproduction

Run from the repository root. Export tab-separated SQL with headers into a
private directory; do not commit database credentials or character data.

- `visual-hotfix-spells.tsv`: hotfix database `SELECT ID FROM spell`.
- `mechanics-proc.tsv`: world database `SELECT * FROM spell_proc`.
- `mechanics-hotfix-counts.tsv`: columns `table_name`, `rows_count`, containing
  counts for the tables below. For each nonempty table, export `SELECT * FROM
  <table>` to `mechanics-<table>.tsv` in the same directory.

```text
talent pvp_talent chr_specialization specialization_spells spell_equipped_items
spell_aura_options spell_procs_per_minute spell_procs_per_minute_mod
spell_cooldowns spell_power spell_reagents_currency spell_totems
skill_race_class_info scenario_step criteria_tree criteria scenario
currency_types totem_category
```

```powershell
python tools/research/audit_spell_mechanics.py --sql-directory .codex `
  --output .codex/mechanics-audit.json
./tools/tests/Test-SpellMechanics.ps1
./tools/tests/Test-GlyphLifecycle.ps1
./tools/tests/Test-SkillLifecycle.ps1
./tools/tests/Test-ProfessionRegression.ps1
./tools/tests/Test-WarlockRegression.ps1
# Apply the hotfix migration, rebuild Release and stop worldserver gracefully:
./tools/tests/Test-WorldShutdown.ps1 -ProbeConnections
```
