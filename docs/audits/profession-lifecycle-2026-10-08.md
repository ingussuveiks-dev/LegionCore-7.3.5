# Profession and glyph lifecycle follow-up — 7.3.5.26972

Offline follow-up to `professions-2026-10-08.md` and
`inscription-class-glyph-2026-10-08.md`. Three additional production defects
were reproduced and fixed. No character data or SQL configuration was changed.

## Fixes and regression coverage

1. **Independent glyphs incorrectly conflicted.** `CheckCast` treated exclusive
   category zero as a shared restriction. Only nonzero categories now exclude
   one another. Native examples are Luminous Charger (GlyphProperties 190,
   aura 89401) and Winged Vengeance (454, aura 57979). Their independent spell
   bindings can coexist. The test extracts the production glyph cast check and
   apply handler; it covers coexistence, same-spell replacement, removal of the
   old aura, client removal notification, inactive-spec isolation, and retained
   nonzero-category/spec/challenge restrictions.
2. **Learning/relearning a profession could leave equipped enchants inactive.**
   `SetSkill` restored enchants before inserting/reactivating the skill status
   and reapplying racial/temporary bonuses. `GetSkillValue` therefore saw zero,
   and threshold checks could also miss bonuses. Restoration now follows status
   and bonus initialization. The test extracts production `SetSkill`, skill
   value/bonus readers, bonus modification and `UpdateSkillEnchantments`.
   It exercises new and previously saved skill states, Engineering 235 + 15
   against a 250 requirement, unlearn/relearn, recipe isolation, and slot reuse.
   The final item-effect application is mocked, including its skill requirement.
3. **Failed item creation could grant guild progress/news.** `DoCreateItem`
   awarded these before checking `StoreNewItem`'s result. The null check now
   precedes guild rewards. The extracted production handler test covers failed
   and successful storage, full/partially full inventory, actual stored quantity,
   creator metadata, skill progress, and a missing item template.

Each new regression failed against the old production handler and passed after
the fix. These are production-handler tests with lightweight fixtures, not a
live character session. Partial inventory storage remains existing behavior;
this change does not promise transactional rollback of an entire craft.

## Native data and database references

`audit_profession_lifecycle.py` checks matching runtime DB2 layout hashes,
copy rows and relationship sections, then applies the relevant exported spell,
spell-effect, item and item-effect SQL overlays. Unsupported nonempty overlays
for the audited skill/glyph/reagent tables stop the audit.

| Checked set | Count |
| --- | ---: |
| Existing profession catalog spells | 6,580 |
| Crafted item IDs | 5,218 |
| Profession reagent records / distinct reagent items | 6,181 / 1,276 |
| Vendor rows / directly sold profession technique items | 75,097 / 1,850 |
| Spells reached through learn relationships | 6,581 |
| Crafted glyphs / glyphs without exclusive category | 104 / 59 |
| Persisted character glyph rows | 0 |

No errors were found in the checked crafted-item references, positive reagent
counts, direct trainer/vendor/quest sources teaching absent catalog recipes,
learn targets/cycles, or crafted glyph properties, aura spells and bindings.
Item overlays contained 1,273 rows and item-effect overlays 84 rows. The adjacent
JSON retains native source hashes and private SQL export hashes, not credentials
or character records. Direct source checks do not prove every scripted/loot
source or every trainer eligibility/rank rule is correct.

Reviewed glyph persistence and spec switching: `_SaveGlyphs` writes all spec
groups in the save transaction; `_LoadGlyphs` filters invalid spec and glyph IDs;
switching specialization removes the old group's auras, applies the new group's
auras and sends a full glyph update. There were no saved glyph rows in the local
database, so a real relog round trip was not exercised. Reviewed reagent checks,
cast-end validation and milling/prospecting quantity checks; no further change
was justified. Interrupted crafting and concurrent inventory mutations were
not simulated end to end.

## Client files and Release runtime

The expanded presentation set (64 files) and enchant item-visual set (45 files)
were combined: **109 distinct FileDataIDs were opened and fully read** from the
installed 7.3.5.26972 CASC storage. All 32 index file SHA-256 hashes were unchanged
across the read. `Test-CascVisualFiles.ps1` preserves read-only access but permits
sharing with the running client's write handles in private CascLib source
copies. Production CascLib and the client installation are unchanged. This
resolves the earlier storage-open failure; it does not verify rendering or all
transitive asset dependencies such as textures referenced from a model.

Both Release executables built successfully in `build-extractors/bin/Release`.
Runtime configs, certificates, DLLs and data were preserved.
`Test-WorldShutdown.ps1 -ProbeConnections` reached ready, probed both world
ports, exited with code 0 and produced no new crash dump. Startup DBErrors.log
still contains the previously documented unrelated waypoint-script 347 warning.

All five suites passed: GlyphLifecycle, SkillLifecycle, CraftingRegression,
ProfessionRegression, and InscriptionGlyph. Skill extraction produces existing
C4244 conversion warnings; assertions pass. In-game visual appearance,
replacement/removal, relog persistence and live crafting remain client
acceptance checks. The 192962 class reward remains the previously documented
compatibility reconstruction; this audit does not authenticate Blizzard's
original twelve-class mapping.

## Reproduction

Run from the repository root. Reuse the private exports documented in
`professions-2026-10-08.md`, refreshing them from the runtime databases, and add:

| Export file | Database / query |
| --- | --- |
| `profession-vendors.tsv` | world: `SELECT entry,item,type,PlayerConditionID FROM npc_vendor` |
| `profession-quest-rewards.tsv` | world: `SELECT ID,RewardSpell FROM quest_template WHERE RewardSpell<>0` |
| `profession-saved-glyphs.tsv` | characters: `SELECT talentGroup,glyphId,COUNT(*) AS n FROM character_glyphs GROUP BY talentGroup,glyphId` |
| `profession-hotfix-items.tsv` | hotfixes: `SELECT * FROM item` |
| `profession-hotfix-item-effects.tsv` | hotfixes: `SELECT * FROM item_effect` |
| `profession-lifecycle-hotfix-counts.tsv` | hotfixes: header `table_name`, `rows_count`; counts for `glyph_properties`, `glyph_bindable_spell`, `glyph_required_spec`, `item`, `item_effect`, `spell_reagents` |

Use tab-separated exports with column headers. An empty saved-glyph export is
accepted. SQL export contents stay private.

```powershell
python tools/research/audit_profession_lifecycle.py --sql-directory .codex `
  --output .codex/profession-lifecycle.json
./tools/tests/Test-GlyphLifecycle.ps1
./tools/tests/Test-SkillLifecycle.ps1
./tools/tests/Test-CraftingRegression.ps1
./tools/tests/Test-ProfessionRegression.ps1
./tools/tests/Test-InscriptionGlyph.ps1
# Supply the visual-chain and profession-item audit JSON reports:
./tools/research/Test-CascVisualFiles.ps1 -ClientPath 'D:/wow/Legion7.3.5' `
  -ReportPaths '.codex/profession-visuals-fixed.json','.codex/profession-audit-fixed.json'
# Stop worldserver gracefully before running this runtime test:
./tools/tests/Test-WorldShutdown.ps1 -ProbeConnections
```
