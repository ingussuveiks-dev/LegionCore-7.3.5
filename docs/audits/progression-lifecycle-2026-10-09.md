# Progression and effect lifecycle follow-up — 7.3.5.26972

This pass covers the six proposed areas through local DB2/SQL inspection,
production-handler regression tests and a Release startup/shutdown check.
It does not certify every quest, combat interaction or client visual in game.
The user's custom boost ship sequence and Broken Shore scripts were preserved.

## Repairs

### Profession item-set requirements and transitions

`AddItemsSetItem` compared the current rank against `RequiredSkill` (the skill
ID), instead of `RequiredSkillRank`. Fourteen native sets have requirements;
for example, set 421 requires skill 197 at rank 300, not rank 197. Native set
requirements range from 300 to 375. Neither ItemSet nor ItemSetSpell has a local
SQL overlay.

The handler now retains equipped-piece counts below the skill requirement but
only applies eligible bonuses. Skill gain/loss and racial/temporary bonus
changes refresh affected sets; unlearning removes their effects, relearning
restores eligible effects without re-equipping, and specialization refresh
cannot reactivate an ineligible set. Already active effects are not recast by
the skill update. Piece thresholds and specialization restrictions still apply.

### Expired heirloom XP aura removal

`ApplyItemEquipSpell` used the current heirloom XP eligibility check on removal
as well as application. An existing effect could therefore survive an unequip
after the character exceeded the permitted level. Eligibility now gates only
application; removal continues to the normal item-specific aura cleanup.

The new extracted-production test failed against the old code on both the rank
boundary and heirloom removal cases, then passed after repair. Additional cases
cover threshold crossings, profession loss/relearn, specialization isolation,
removing pieces while ineligible, and unrelated skills. Lower-level aura casting
is mocked; this is not an equipped-character gameplay session.

### False waypoint-script warning

The startup reference query inspected only `waypoint_data`. Broken Shore's
scripted flight path 439155, point 8, references action 347 from
`waypoint_data_script`. The action exists and despawns the completed flyby.
The query now unions both path tables; no path or action was deleted or changed.
A temporary-table SQL fixture exercises the production query, duplicate actions,
the scripted-only action and a genuinely unreferenced action. The new server
startup produced no additional DBErrors lines; old log history was retained.

## Data and progression checks

- All 14 boost scenarios retain 186 steps. None overlaps retired test steps
  1947/2233/2234. Runtime instance bindings for 1554/1557 and 1460, the 227058
  departure script, and faction routes 786/1189 remain present.
- Broken Shore reentry tests pass, including saved LFG group restoration and
  entrance recovery. The Legion intro suite passes 76 quest/phase checks,
  including the boost route and shared Alliance/Demon Hunter handoffs.
- 30,316 quest templates, 29,930 addon rows, 27,727 objectives and 31,587 distinct
  NPC/gameobject quest relationships were checked for selected references.
  No missing quest-chain target, reward item, reward currency or NPC/gameobject
  questgiver reference was found. All thirteen missing objective targets and
  the one missing start item belong to already disabled quests.
- Creature references use `creature_template_wdb`, which actually seeds
  `ObjectMgr`'s template store. The separate `creature_template` table does not
  provide the complete set of available creature IDs.
- The simple positive-PrevQuest graph contains 14 cycles. Nine have incoming
  `NextQuestID` edges from outside the cycle. `QuestData` adds those as alternate
  prerequisites, so these are not proven deadlocks. Five isolated self-links
  (9679, 32426, 34315, 38923, 39090) remain unmodified: removing a deliberate gate
  without tracing its scripts and availability would enable unverified content.
- 180,465 SpellMisc and 269,891 SpellEffect rows were checked for range,
  cast-time, duration and radius references. No native missing range/radius/time
  reference was found. Five unresolved custom SQL references remain: SpellMisc
  450248 (spell 305248) uses missing range 16; SpellEffects
  410234/410235/410236/410290 use missing radius 3 for spells
  305234/305235/305236/305290. These are custom content, not retail ID replacements.
  The intended numeric range/radius was not established, so it was not invented.
- 2,021 item-set bonus rows resolve their set and spell targets. The 29,206
  ItemEffect rows contain 775 native missing-spell references; 4,935 aura
  restriction rows contain 291 native missing-aura references. These raw
  historical candidates are retained in the JSON. This pass does not establish
  that each is reachable or broken, and does not bulk-delete native records.

The audit overlays current SQL data and applies hotfix tombstones before checking
references. Export counts and SHA-256 hashes accompany the report. No saved
character data was modified. No claim of retail numeric parity follows merely
from finding a referenced row.

## Lifecycle and runtime validation

Passed production-handler suites: ItemEffectLifecycle, SkillLifecycle,
ProfessionRegression, BrokenShoreReentry, LegionIntroSequence, PetLoadSelection,
WarlockRegression (including mover handoff), CastResources, SpellMechanics and
GlyphLifecycle. The separate WaypointScriptReferences and ChargePersistenceSql
fixtures also pass. Charges retain millisecond recovery values and character
isolation; the handler suite checks immediate relog, partial offline recovery,
full offline recovery and invalid saved spell IDs.

Reviewed death and map-transfer aura cleanup: death preserves passive and
death-persistent auras while removing ordinary auras with the death removal mode;
map transfer removes auras flagged for map/movement interruption. Pet resummoning
remains in movement completion. These are source reviews plus scoped regression
tests, not a full simulated death/transport/relog matrix.

Release worldserver and bnetserver built in `build-extractors/bin/Release`.
The existing runtime files and extracted data were preserved. The shutdown test
reached ready, probed both world ports, exited 0 and produced no new crash dump.
Both servers were then restarted; worldserver reached ready and ports
1119/8085/8086 were listening. Existing unrelated compiler warnings remain.

Remaining acceptance: the complete client boost flight and island continuation,
camera/pet portrait, actual equipped-set effects, rendered visuals, and combat
target/immunity behavior. The unresolved custom data and historical reference
candidates above require separate behavioral evidence before any replacement.

## Reproduction

Run from the repository root, using the configured Python interpreter:

```powershell
./tools/research/Export-ProgressionAudit.ps1
python tools/research/audit_progression_lifecycle.py --output .codex/progression-audit.json
./tools/tests/Test-ItemEffectLifecycle.ps1
./tools/tests/Test-SkillLifecycle.ps1
./tools/tests/Test-ProfessionRegression.ps1
./tools/tests/Test-WaypointScriptReferences.ps1
./tools/tests/Test-ChargePersistenceSql.ps1
./tools/tests/Test-BrokenShoreReentry.ps1
./tools/tests/Test-LegionIntroSequence.ps1
./tools/tests/Test-PetLoadSelection.ps1
./tools/tests/Test-WarlockRegression.ps1
./tools/tests/Test-CastResources.ps1
./tools/tests/Test-SpellMechanics.ps1
./tools/tests/Test-GlyphLifecycle.ps1
# Only after gracefully stopping worldserver:
./tools/tests/Test-WorldShutdown.ps1 -ProbeConnections
```
