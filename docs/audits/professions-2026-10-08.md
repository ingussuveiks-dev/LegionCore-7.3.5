# Legion 7.3.5 profession audit — 2026-10-08

## Result

Five server defects fixed and covered by extracted-production-code regression
tests. Release binaries were rebuilt in `build-extractors/bin/Release`.
This is a data/code audit, not an in-client certification of every profession.
The original audit left one Inscription reward chain unresolved. A subsequent
[192962 compatibility repair](inscription-class-glyph-2026-10-08.md) implements
all twelve class rewards and makes the updated profession audit pass. Its
mapping is reconstructed; original Blizzard choices and in-game visuals are
not fully confirmed. Counts and JSON below describe the original audit snapshot.

### Fixes

1. **Alchemy specialization changes:** learning Elixir Master (28677) removed
   itself instead of Transmutation Master (28672). Switching now removes the
   other two masteries. Relearning the same mastery does not strip its aura.
   All three masteries are present under Alchemy (171) in the 26972 DB2.
2. **Skill-value rewards:** `learnSkillRewardedSpells` skipped already-known
   spells before testing their required rank. Lowering a skill could leave its
   rank-dependent spells known. Check the threshold first, remove through the
   existing `removeSpell` path, and retain the known-spell shortcut for rewards
   whose requirements still hold. Explicitly learned recipes are unaffected by
   this rank-reward check. Tests use Inscription research 165304 (75) and 165456
   (100), including lowering below the threshold and learning again.
3. **Invalid enchant records:** one missing enchant DB2 entry caused
   `UpdateSkillEnchantments` to return, skipping later items and sockets.
   Continue past the invalid entry so remaining valid effects are updated.
4. **Profession bonuses:** enchant threshold transitions used the base skill,
   while `ApplyEnchantment` checks skill including temporary/permanent bonuses.
   For example, a character with Engineering 235 plus a 15-point bonus could
   retain Nitro Boosts (enchant 4223, requirement 250) when unlearning.
   Threshold transitions now include the same bonuses, with effective rank zero
   when the skill is removed. The existing ordering removes effects before
   `SetSkill` clears the rank and bonus fields.
5. **Shared Cooking recipes:** `HandleShowTradeSkill` inserted the parent ID
   again instead of each known child ID. It now includes the player's actual
   child skills and their ranks. Native children are the six Pandaria Ways
   (975–980), Apprentice Cooking (981), and Journeyman Cookbook (982).

## Data coverage

Sources are the canonical runtime `dbc/enUS` WDC1 files, matched to the repository
layout hashes for build **7.3.5.26972**, plus runtime SQL exports. Source hashes
and detailed findings are in `professions-2026-10-08.json`.

| Check | Coverage / result |
| --- | --- |
| Profession families | 11 primary, 4 secondary, 8 Cooking child skills |
| Unique profession catalog spells | 6,639 |
| Existing spells reached through effects, learning links and relearn scripts | 6,766 |
| Profession-required enchant records | 96 |
| Enchant records referenced by profession effects or profession requirements | 468; no missing records |
| Enchant ItemVisual references | 49; no missing records |
| Direct enchant model files | 45; all fully read from installed client CASC |
| Spell visuals / kits | 54 / 72; no missing visual/kit records |
| Visual attachments / effect names | 61 / 40; no missing records |
| Direct spell model/texture files | 38; all fully read from installed client CASC |
| Legion profession quest relearn scripts | 11; all bound and EFFECT_0 / DUMMY hooks match runtime effects |
| Skill/ability/learn/enchantment/ItemVisual hotfix tables | Empty; native relationships apply |

The visual scan reuses `audit_character_spell_chains.py` with the profession
roots. Its raw missing-spell diagnostics are classified below; none are missing
visual assets. A spell without its own visual is not automatically broken:
crafting/research/passive/learning and removal effects have different presentation
requirements. Model readability does not establish that an animation displays
correctly in the client or that every nested texture dependency exists.

## Missing references and limits

- **59 native SkillLineAbility spell references** have no effective Spell entry:
  58 Inscription and one Enchanting. No exported `npc_trainer` row teaches those
  IDs. They are recorded, not recreated from guessed spells. This does not prove
  every item/quest/script source avoids them.
- **18 missing targets are REMOVE_AURA (140)** effects. These are cleanup
  references, not missing spells to cast or learn; leave the native data intact.
- **Spellcloth 31373 → 31374** is handled explicitly by
  `AuraEffect::HandlePeriodicTriggerSpellAuraTick`: it summons creature 17870.
- **Originally unresolved: Recipe: Class Glyph (192962)** has twelve TRIGGER_SPELL (64)
  effects targeting absent IDs 192969–192976 and 192978–192981. The live world
  database gives quest 39931 this reward, and the Inscription relearn script
  casts it for rewarded quest 39931. No C++ handler or exported dummy/aura-trigger
  override for it was found. Missing client rows can represent server-only data;
  they are not proof that the reward should be deleted. Correct class-to-recipe
  mapping requires authoritative 7.3.5 server data or a matching client/server
  capture. The subsequent compatibility repair handles this chain with a
  documented reconstruction; exact original class choices remain unconfirmed.

Reviewed unlearning flow: `HandleUnlearnSkill` checks the native unlearnable flag,
then `SetSkill(id, 0)` removes skill-dependent enchant effects, clears skill and
bonus fields, removes SkillLineAbility spells via `removeSpell`, and clears the
profession slot. `removeSpell` handles the spell's own auras, learned dependencies,
and first-rank profession points. No blanket deletion of consumable buffs,
crafted items, quest reward history, or all spells created by a profession was
introduced.

The tests exercise production handlers with lightweight fixtures, not a full
Player/database session. No saved player professions were changed. Client tests
still needed: unlearn/relearn each profession, relog persistence, profession-slot
reuse, racial/temporary bonuses on equipped items, socket stats, Cooking sharing,
crafting/gathering animations and quest-recovery books. The unresolved glyph
reward prevents claiming complete profession correctness.

## Verification

- `tools/tests/Test-ProfessionRegression.ps1`: pre-fix assertions failed for the
  four initial defects and separately for the bonus-threshold defect; all pass
  after fixes. Covers all nine mastery transitions, rank loss/relearning, invalid
  enchant continuation, prismatic sockets, bonus thresholds and Cooking sharing.
- Release build: `worldserver` and `bnetserver`, canonical runtime directory.
- `tools/tests/Test-WorldShutdown.ps1 -ProbeConnections`: server ready,
  both port probes successful, clean shutdown, no new crash dumps.
- Startup still reports the pre-existing unrelated waypoint-script 347 warning.

## Reproduction

Run from the repository root with Python 3. Export tab-separated tables with
column headers to a private directory; no credentials or character data belong
in the repository.

| Export file | Database / query |
| --- | --- |
| `visual-hotfix-spells.tsv` | hotfixes: `SELECT ID FROM spell` |
| `visual-hotfix.tsv` | hotfixes: `SELECT * FROM spell_x_spell_visual` |
| `chain-hotfix-effects.tsv` | hotfixes: `SELECT * FROM spell_effect` |
| `profession-world-learn.tsv` | world: `SELECT * FROM spell_learn_spell` |
| `profession-trainer.tsv` | world: `SELECT * FROM npc_trainer` |
| `chain-script-bindings.tsv` | world: `SELECT spell_id, ScriptName FROM spell_script_names` |
| `chain-spell_dummy_trigger.tsv` | world: `SELECT * FROM spell_dummy_trigger` |
| `chain-linked-spells.tsv` | world: `SELECT * FROM spell_linked_spell` |
| `chain-spell_pet_auras.tsv` | world: `SELECT * FROM spell_pet_auras` |
| `profession-hotfix-counts.tsv` | hotfixes: header `table_name`, `rows_count`; counts for `skill_line`, `skill_line_ability`, `spell_learn_spell`, `spell_item_enchantment`, `item_visuals` |

```powershell
python tools/research/audit_professions.py --sql-directory .codex `
  --output .codex/profession-audit.json --visual-roots .codex/profession-visual-roots.json
python tools/research/audit_character_spell_chains.py `
  --audit .codex/profession-visual-roots.json --sql-directory .codex `
  --output .codex/profession-visuals.json
./tools/tests/Test-ProfessionRegression.ps1
```

The original first audit returned exit code 1 for twelve unresolved glyph
targets. After the follow-up repair and refreshing the script binding export,
the updated profession audit returns 0. The reused raw visual scanner continues
to report native cleanup/fallback references; see the follow-up for classification.
