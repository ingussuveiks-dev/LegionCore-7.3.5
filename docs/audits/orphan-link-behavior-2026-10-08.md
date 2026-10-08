# What replaces the four orphan links in 7.3.5?

The previous pass proved that four targets were absent, but did not trace their
intended behavior far enough. This follow-up identifies the old priest spell,
checks current functional coverage, fixes a visual-controller initialization gap,
and distinguishes test scenarios from the active Broken Shore routes.

## 194248 was a visual controller, not a missing combat button

SimulationCraft's historical generated data names spell 194248 **Insanity Visual
Controller** and includes it in the Shadow specialization list. The pinned
snapshot is dated 2016-02-28, client build 21154. Its seven aura effects use aura
328, Insanity power type 13, and trigger 210195 around power thresholds.
Sources: [spell data](https://raw.githubusercontent.com/simulationcraft/simc/5d5e067b67298309c4c97909e7690e90dafe2ea9/engine/dbc/generated/sc_spell_data.inc),
[specialization list](https://raw.githubusercontent.com/simulationcraft/simc/5d5e067b67298309c4c97909e7690e90dafe2ea9/engine/dbc/generated/sc_spell_lists.inc).
Hashes are in the adjacent JSON; the snapshot is historical evidence, not data
to import into a 26972 server.

Current native specialization entries already teach:

| Entry | Spell | Role |
| --- | --- | --- |
| 5434 | 228260 | Void Eruption |
| 5435 | 228266 | Void Bolt spellbook entry |
| 5436 | 228264 | Voidform spellbook entry |
| 5576 | 232698 | Shadowform and its server visual controller |

The actual Voidform aura is 194249, applied by Void Eruption's script. It replaces
the cast with Void Bolt 205448 through its native override effect. Substituting
194249 into the old learning link would teach a triggered aura directly.
Substituting 232698 would duplicate existing entry 5576. Neither is needed.
An extracted production `LearnSpecializationSpells` fixture checks that removing
the missing controller link preserves the four current learning entries.

Native Shadowform 232698 still has an effect triggering absent spell 210195.
Neither 194248 nor 210195 is restored by local SQL. The existing
`spell_pri_shadowform` AuraScript supplies the visual-controller behavior using
the present 7.3.5 visual auras 185908–185911. All four spells and their sixteen
race-conditioned SpellXSpellVisual targets exist in the runtime DB2 files.
This establishes current functional coverage; it is not an official one-to-one
spell-ID renumbering claim.

**Repair:** the script previously waited for its first 1,000 ms update before
applying visuals. With the old trigger unavailable, entering or re-entering
Shadowform had no immediate scripted initialization. `AfterEffectApply` now
initializes the visual state from current Insanity. The existing update and
cleanup paths remain shared: base stage at 0, further stages at 25/50/75 Insanity,
and all four auras removed when Shadowform ends. No obsolete controller is taught
or reconstructed from alpha-era data.

## The three scenario steps have no demonstrated playable replacement

- Step 1947 belongs to scenario 908, **Test Faction Criteria**, whose first step
  1826 uses tree 44372. Its final-step tree 31016 is absent. LFG 1027 explicitly
  references this test scenario; that does not make it the live Broken Shore.
- Steps 2233/2234 refer to missing scenario 1045. Their surviving roots are
  46960 (test progress bar) and 46963, explicitly labeled as a test to kill ten
  Mook Laborers. Child 49510 uses Criteria 30991, kill-creature type 0, asset
  109970 and amount 10. These are concrete test objectives, not evidence for a
  replacement quest or dungeon.
- Local `scenario_data` routes map 1460 to **786 for Alliance (469)** and
  **1189 for Horde (67)**. Neither 908 nor 1045 has local scenario/step-spell
  bindings. The native LFG 908 → Scenario 786 relationship also illustrates why
  IDs from different tables must not be substituted just because numbers match.

The current native rows, tree descriptions and routing were checked directly;
selected numeric rows are in the JSON. Source labels come from the version-pinned
CSV evidence linked in the [previous audit](cast-resources-2026-10-08.md).
No replacement for these test objectives was established. Reattaching them to
786/1189 would introduce test objectives into playable content. The existing
three exact test-step tombstones therefore remain; no gameplay step is invented.
This does not claim to verify the complete Broken Shore scenario in game.

## Validation

- `Test-ShadowVisualController.ps1` compiles the production AuraScript: the old
  handler fails immediate initialization and re-entry; the repaired handler
  passes. Coverage includes boundary values below/at 25, 50 and 75 Insanity,
  rising/falling power, idempotent updates, full cleanup and absent caster.
- `Test-SpellMechanics.ps1` checks the current Shadow learning entries alongside
  existing specialization, equipment and aura regressions.
- `Test-CastResources.ps1` remains green.
- Release `worldserver`/`bnetserver` build passed in the canonical
  `build-extractors/bin/Release` directory. Existing enum/float conversion
  warnings in unrelated priest handlers remain.
- `Test-WorldShutdown.ps1 -ProbeConnections` passed: ready reached, exit 0,
  no new crash dumps. Runtime configurations and extracted data were preserved.

Rendering and real character transitions still need the 7.3.5 client. The tests
verify emitted aura operations and dependency existence, not displayed pixels.

## Additional archive search

A further search found the same unresolved scenario links in an independent
CSV archive pinned to commit `472d3a30957eb702a319a73e489407489d5aa429`, build
10.0.2.47067. Step 1947 still targets tree 31016; steps 2233/2234 still target
scenario 1045. Neither target exists in that snapshot's corresponding table.
The test roots 46960/46963 still have the same labels. Sources:
[ScenarioStep](https://raw.githubusercontent.com/maxdekrieger/wow-csv-from-db2s/472d3a30957eb702a319a73e489407489d5aa429/versions/10.0.2.47067/csv/scenariostep.csv),
[Scenario](https://raw.githubusercontent.com/maxdekrieger/wow-csv-from-db2s/472d3a30957eb702a319a73e489407489d5aa429/versions/10.0.2.47067/csv/scenario.csv),
[CriteriaTree](https://raw.githubusercontent.com/maxdekrieger/wow-csv-from-db2s/472d3a30957eb702a319a73e489407489d5aa429/versions/10.0.2.47067/csv/criteriatree.csv).

This is corroboration of long-lived leftover references, **not** permission to
import modern game data or proof that no historical replacement ever existed.
Version-pinned rows and hashes are appended to the JSON evidence.

Targeted GitHub code, commit and issue searches in TrinityCore, AshamaneCore,
dufernst/LegionCore, The Legion Preservation Project and LegionCore-Reforged
did not identify a repair mapping for these references. Search indexes are not
an exhaustive examination of every historical branch. The old wow.tools export
endpoint for Scenario build 7.0.3.22248 returned HTTP 404, so the original early
Legion parent/tree records remain unavailable in this investigation.

No new runtime/data patch follows from these findings. The unresolved question
is the original content of Scenario 1045 and CriteriaTree 31016, not an established
missing step in the current playable scenarios.
