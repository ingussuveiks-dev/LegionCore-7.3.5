# Inscription 192962 compatibility repair — 2026-10-08

Spell 192962, **Recipe: Class Glyph**, previously attempted twelve missing
trigger spells when rewarding quest 39931 (Smashing Herbs) or using the
Inscription relearning book. The new `spell_gen_inscription_class_glyph` script
suppresses those triggers in both launch phases and teaches one class recipe
after the cast. It does not cast the crafting spell or give free crafted items.
The same reward spell remains in the quest and existing relearning script.

**This is a reconstruction, not a fully confirmed reproduction of Blizzard's
server-only class mapping.** All twelve selected recipes and their glyph chains
are compatible with the installed 7.3.5.26972 DB2 data. The original reward
choice for every class could not be established from the available sources.

## Evidence and selection

- The runtime SpellEffect table has effects 0–11, all TRIGGER_SPELL, referencing
  192969–192976 and 192978–192981. These children have no native Spell records;
  no matching local hotfix or implemented script was found.
- [The reward's description and trigger list](https://www.wowhead.com/spell=192962/recipe-class-glyph)
  agree with the class-specific intent and missing intermediate spell IDs.
- [Varenne's first-person quest report](https://www.wowhead.com/quest=39931/smashing-herbs)
  explicitly identifies Glyph of the Queen for a paladin. It does not establish
  all twelve classes or constitute a 26972 server capture.
- Native ItemEffect records for techniques 137730–137740 identify ten extant
  class recipes, 192838–192846 and 192848. The intervening 192847 is absent.
  Selecting the ten extant entries as the original class rewards is an inference.
- Warrior uses the existing Legion vendor recipe Blazing Savior (225560).
  Demon Hunter uses Fel-Enemies (225528). These are explicit compatibility
  choices, not a claim that the missing IDs were renamed to these numbers.
  The [Legion Inscription guide](https://www.wowhead.com/guide/legion-inscription)
  lists both as Jang Quillpaw recipes. Fel Touched Souls is excluded because
  the profession chain awards it separately for The Price of Power.

| Class | Recipe spell | Recipe | Crafted item |
| --- | --- | --- | --- |
| Warrior | 225560 | Glyph of the Blazing Savior | 137188 |
| Paladin | 192846 | Glyph of the Queen | 137293 |
| Hunter | 192845 | Glyph of Stellar Flare | 137269 |
| Rogue | 192841 | Glyph of Blackout | 139358 |
| Priest | 192838 | Glyph of Ghostly Fade | 129017 |
| Death Knight | 192848 | Glyph of the Wraith Walker | 139274 |
| Shaman | 192844 | Glyph of the Spectral Raptor | 137287 |
| Mage | 192840 | Glyph of Sparkles | 129019 |
| Warlock | 192839 | Glyph of Fel Imp | 129018 |
| Monk | 192843 | Glyph of Crackling Crane Lightning | 139338 |
| Druid | 192842 | Glyph of the Sentinel | 129021 |
| Demon Hunter | 225528 | Glyph of Fel-Enemies | 139437 |

## Lifecycle and verification

- Recipe learning requires Inscription and is idempotent. Unsupported classes
  and non-player casters receive nothing. All selected recipe IDs are validated
  at script load. The relearning path retains its completed-quest check.
- Each recipe belongs to SkillLine 773. Existing `Player::SetSkill(..., 0)`
  removes that skill's recipes through `removeSpell(GetFirstSpellInChain(...))`.
  This code path was reviewed; the new handler does not add a persistent aura.
  Applying or removing a crafted cosmetic glyph is a separate client workflow.
- `Test-InscriptionGlyph.ps1` compiles the actual reward and relearning classes:
  all twelve classes, both launch hooks, duplicate rewards, absent profession,
  incomplete quest, restored recipe, invalid class/caster and missing recipe data.
- `Test-ProfessionRegression.ps1` passes the existing mastery, skill-rank,
  enchant cleanup and profession sharing checks.
- `audit_inscription_class_glyph.py` verifies all twelve native recipe/skill,
  crafted item/class, item-use APPLY_GLYPH, GlyphProperties and bindable spell
  relationships, plus the SQL binding and script registration. Relevant
  spell-effect SQL overlays fail closed for manual review.
- `audit_professions.py` now checks the reconstructed handler before classifying
  its twelve missing native targets as handled. It reaches 6,809 existing spells
  and reports no unresolved trigger targets or data errors.
- The raw presentation scan reaches 79 visuals, 118 kits, 117 attachments and
  67 effect-name records without missing visual records. It still prints the
  31 raw missing spell references (18 removal-only, one Spellcloth fallback,
  twelve suppressed class triggers); its exit code is therefore still 1.
- Release `worldserver.exe` and `bnetserver.exe` were built in the canonical
  `build-extractors/bin/Release` directory. Startup/port/shutdown regression
  passed. Startup reports only the previously documented waypoint 347 warning
  in DBErrors.log, with no new script validation error.

The current CASC probe could not open the client storage (error 2); the expanded
64-file presentation set is **not** certified by a successful full-read run.
The previous profession audit's 38-file result must not be extended to that set.
The launched client reports 7.3.5.26972, but remains at login pending user input.
No in-game recipe reward, crafting, glyph appearance, replacement or removal
test has been completed in this follow-up. Do not describe visuals as verified.

## Reproduction

Apply `sql/updates/world/2026_10_08_00_inscription_class_glyph.sql` and restart the
rebuilt server. It was also applied to the local runtime database. Refresh the
private SQL exports listed in `professions-2026-10-08.md`, especially
`chain-script-bindings.tsv`, then run:

```powershell
./tools/tests/Test-InscriptionGlyph.ps1
./tools/tests/Test-ProfessionRegression.ps1
python tools/research/audit_inscription_class_glyph.py --sql-directory .codex `
  --output .codex/inscription-glyph-audit.json
python tools/research/audit_professions.py --sql-directory .codex `
  --output .codex/profession-audit-fixed.json
```

Hashes, native IDs, and the follow-up audit counts are retained in the adjacent
`inscription-class-glyph-2026-10-08.json` report. In-client acceptance should
check reward, unlearn/relearn, craft, apply, replace and remove for each class.
