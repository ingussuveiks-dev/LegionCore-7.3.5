# Legion 7.3.5 spell visual audit

The local build 26972 DB2 files and the effective SQL visual overlays were
checked on 2026-10-08. Four hotfix records displaced native visual mappings;
the migration restores those mappings while preserving the custom spells.
The server's optional visual-table loader now loads each table independently
and clears visual kits on reload.

## Coverage and results

| Check | Result after repair |
| --- | ---: |
| Effective Spell IDs including SQL spell overlays | 179,641 |
| Effective SpellXSpellVisual rows | 110,336 |
| Available SpellVisual / SpellVisualKit IDs including DB2 copies | 63,360 / 81,113 |
| Skill-line, specialization, talent and saved-character spell candidates | 10,374 |
| Class-mask, specialization and talent subset | 1,804 |
| Distinct saved character spells | 106 |
| Conflicting native visual-row ownership | 0 |
| Nonzero visual IDs absent from the 26972 client table | 0 |
| World visual and visual-kit SQL rows checked | 90, all assets present |
| Literal visual-kit references in spell scripts checked | 17, all assets present |

The asset tables have no SQL overlays, and the visual-link table has no
deletion hotfixes in this installation. The audit script only overlays the
provided spell-ID and spell-visual exports; it does not apply arbitrary
hotfixes to other DB2 tables. Its JSON includes input DB2 SHA-256 hashes.

## Repaired mappings

| Native visual row | Restored spell | Native visual | Displaced custom spell | New custom row |
| --- | --- | ---: | ---: | ---: |
| 120001 | 244509 Sorority of the Sargerei | 33100 | 305005 | 400005 |
| 120002 | 244510 Calming Wave | 66986 | 305008 | 400008 |
| 120003 | 243995 Calmed State | 67149 | 305009 | 400009 |
| 120004 | 244477 Lingering Flames | 67148 | 156333 | 400333 |

SQL hotfix loading overwrites DB2 records by row ID. Reusing these native IDs
changed the owning spell and removed its original association. The new custom
IDs are above the client table's maximum ID of 251498. Fresh, unique hotfix
notifications cover all eight records, including restored native records
that a client might have cached. Custom visual values are preserved.

`SpellMgr::LoadSpellVisual` also returned early when either preceding optional
table was empty, so orphan visuals or kits could be skipped. Repeated loads
appended duplicate kits because only the other two maps were cleared. Both
paths are covered by a test compiled from the production function.

## Interpretation and remaining client checks

A missing direct SpellXSpellVisual row does not by itself mean an effect is
broken. Of the 106 saved character spells, 50 have a direct mapping and 49
are passive spells without one. The remaining seven are Rising Sun Kick,
Counter Shot, Honorable Medallion and four profession entries. Rising Sun
Kick 107428 triggers 185099, which has visual row 83557 / visual 39941.
Counter Shot's native effect is interrupt (68); Honorable Medallion uses
mechanic immunity (aura 77), with a core handler that removes movement/control
impairments. No invented visuals were added to these entries.

The broad raw candidate set retains 195 IDs absent from Spell, including
117 in the class subset. SpecializationSpells row 4946 references absent
spell 194248 for Shadow specialization 258; `Player::LearnSpecializationSpells`
skips absent SpellInfo records. No talent spell is absent. These inherited
client references are reported rather than assigned speculative replacements.
All 106 saved character spell IDs exist in the effective catalogue.

Forty existing non-passive class candidates lack a direct mapping. The JSON
lists them for checking triggered spells, scripted effects and client
animations. The [follow-up chain audit](character-spell-chains-2026-10-08.md)
reviews all 40, verifies their reachable model/texture assets and fixes a
duplicate Death Coil dispatch. The inherited same-owner hotfix for Warrior Skyjumped 247860
changes visual 8742 to 7723; both exist in 26972, and this audit does not
establish that the override is erroneous. It is preserved.

This establishes reference integrity, not that every animation renders
correctly in game. All-class client casting, glyph/condition variants,
missiles, impacts and aura removal still require gameplay verification.
Client asset-file completeness and visual appearance were not verified.

## Validation

- Migration applied twice to temporary copies: four native mappings restored,
  four custom mappings preserved, eight unique notifications each time.
- `tools/tests/Test-SpellVisualLoading.ps1` passed with empty-table, reload,
  removal and invalid-spell cases.
- Release `worldserver` and `bnetserver` built successfully in
  `build-extractors/bin/Release`; runtime data and configuration preserved.
- The normal updater applied the migration to the local hotfix database.
- `tools/tests/Test-WorldShutdown.ps1 -ProbeConnections` passed: current-run
  readiness, greetings on both ports, exit 0 and no crash dumps. Its readiness
  check was fixed to exclude historical ready lines in the append-only log.
- The final strict visual audit passed. Startup still reports the pre-existing
  unrelated orphan waypoint script 347; there were no spell-visual errors.

## Reproduction

Run from the repository root with Python 3. Export tab-separated data with
column headers using these queries on the respective databases:

```sql
-- hotfixes
SELECT * FROM spell_x_spell_visual;
SELECT ID FROM spell;
-- characters
SELECT DISTINCT spell FROM character_spell ORDER BY spell;
-- world
SELECT spellId, SpellVisualID FROM spell_visual
UNION ALL SELECT spellId, SpellVisualID FROM spell_visual_play_orphan;
SELECT spellId, KitRecID FROM spell_visual_kit;
```

```powershell
python tools/research/audit_spell_visuals.py --hotfix-visuals visuals.tsv --hotfix-spells spells.tsv --character-spells characters.tsv --world-visuals world-visuals.tsv --world-kits world-kits.tsv --output audit.json --strict
```

Optional SQL inputs can be omitted for a DB2-only inventory. Strict mode fails
on hotfix ownership collisions and missing referenced visual assets, not on
passives, wrapper spells or inherited absent spell IDs.
