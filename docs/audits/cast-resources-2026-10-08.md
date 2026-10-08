# Cast resources, charge persistence and orphan DB2 links — 7.3.5.26972

Follow-up: [intended behavior behind the orphan links](orphan-link-behavior-2026-10-08.md)
identifies 194248 as the old Insanity visual controller, verifies current Shadow
learning/visual coverage, repairs delayed visual initialization and checks actual
scenario routing. Target absence alone was not a complete behavior audit.

This follows the [spell mechanics audit](spell-mechanics-2026-10-08.md).
Confirmed resource and charge defects are repaired. The four previously
unresolved native references are retired as stale/test links, without inventing
replacement spells or scenario behavior. Client acceptance remains pending.

## Resource checks and payment

- `CheckPower` returned success immediately after a sufficient health check,
  bypassing other resource requirements. It now checks every active resource.
  Native spells 45902, 206930 and 228645 combine health (conditional on aura
  202846), runes and generated runic power.
- Repeated preparation/completion checks appended to `m_powerData`. Clearing
  this list before each population prevents duplicated rows and repeated holy
  power handling, and removes rows whose required aura disappeared.
- `CalcPowerCost` resolves one final cost per resource, but `TakePower` deducted
  that value once per eligible DB2 row. Conditional and unconditional rows for
  the same resource can coexist (for example, energy rows for 116095). Payment
  and its script hook now run once per resource, preserving mixed-resource casts.

The tests compile the actual production handlers. The old preflight fails mixed
health/mana and repeated holy-power checks; the old payment handler fails the
duplicate energy-row fixture. All pass after repair. The full cast pipeline is
not mocked: review confirmed that final `CheckCast(false)` failure returns before
charge, cooldown, power and reagent consumption. Live interrupted casts still
need client acceptance.

Production `TakeReagents` tests pass for reagent and currency amounts, exemptions,
and a casting item which is also a consumed reagent. The last-charge case clears
casting-item, target and GUID references before destruction. No reagent handler
change was necessary. Equipment-dependent aura removal/cast cancellation and
passive equipment requirements were reviewed alongside the previous armor tests;
no additional proven defect was found in this pass.

## Charge recovery and persistence

Repairs cover negative recovery modifiers wrapping unsigned durations, missing
spell pointers in script-created charge state, charge-count overflow/underflow,
negative timer changes awarding charges, large reductions restoring only one
charge, and snapshots using the native duration instead of the effective one.
Recovery now consumes the completed interval before calculating the next one;
changing recovery duration cannot subtract the wrong interval. Full categories
report zero remaining recovery.

Charges previously existed only in player memory. The new
`character_spell_charges` table saves outstanding category charges transactionally
with the character, including the next recovery timestamp in milliseconds and
effective interval. Login restores partial progress and accounts for offline
time. Fully recovered, unknown-spell and invalid-category records are ignored;
character deletion removes persisted charge rows. Offline recovery uses the
saved effective interval until the next online interval is calculated.

Apply `sql/updates/characters/2026_10_08_001_persist_spell_charges.sql` to the
characters database **before starting the new server**. It was applied locally;
the base schema also includes the table. Tests cover immediate reload, partial
offline recovery and complete recovery using extracted save/load handlers.
A real SQL insert/read was rolled back and verified to leave zero fixture rows.
This is not an end-to-end player login/reconnect test.

## Evidence for retiring the four references

The adjacent JSON records source URLs, SHA-256 hashes and selected source rows.
Downloaded 7.3.5.25600 and 7.3.5.26972 CSVs were compared with local 26972 WDC1
data and SQL overlays.

| Link | Evidence and disposition |
| --- | --- |
| SpecializationSpells 4946 → Spell 194248 | The same link exists in both builds, but the spell is absent in 25600 Spell CSV and local 26972 Spell data/SQL. Existing specialization learning already skipped its missing SpellInfo. Retire this dangling link. |
| ScenarioStep 1947 → CriteriaTree 31016 | Scenario 908 is explicitly named `Test Faction Criteria`, with area 0. The step exists in both builds; its tree is absent in current 26972 data/SQL. Retire the test step. |
| ScenarioStep 2233 → Scenario 1045 | Scenario 1045 is absent in both builds. The step's text is test/tutorial instructions, not an identified playable scenario. Retire the orphan step. |
| ScenarioStep 2234 → Scenario 1045 | Same absent parent and explicit final-step test instructions. Retire the orphan step. |

Local `scenario_data` and `scenario_step_spells` contain no bindings for 908 or
1045. There **is** a native LFG reference: entry 1027 points to 908 and is named
`Test Scenario Faction Criteria`. Therefore this is not a claim that scenario
908 has no references. No playable replacement behavior has been established.

`sql/updates/hotfixes/2026_10_08_003_retire_orphaned_client_links.sql` adds four
supported deletion notifications, guarded against SQL overrides/restored parents.
It does not edit extracted DB2 files or unrelated records. Reapplying it locally
left the notification export unchanged. Table hashes come from WDC1 headers.
The specialization index is built before hotfix deletion, so a production fix
also prunes deleted entries from that derived index. Its extracted test preserves
unrelated live entries while removing the tombstoned link.

The audit now honors exported deletion notifications and reports zero findings
for its reference checks. Effective counts are 898 SpecializationSpells and
1,634 ScenarioStep records. The latter also honors an already-existing tombstone
for step 409; this migration does not delete that step again. These results do
not establish that every spell or scenario in the game behaves correctly.

## Validation and reproduction

- `Test-CastResources.ps1`: all three extracted-production C++ suites pass.
  Old handlers failed ten resource/recovery assertions and the separate duplicate
  payment assertion. Save/load coverage is new rather than a pre-fix executable
  comparison, since those functions did not exist.
- SpellMechanics, SkillLifecycle, GlyphLifecycle, CraftingRegression,
  ProfessionRegression and WarlockRegression (including mover checks) pass.
- Final Release `worldserver` and `bnetserver` builds pass in
  `build-extractors/bin/Release`; runtime configs, certificates, DLLs and data
  were preserved. Existing conversion warnings in extracted handlers remain.
- Final `Test-WorldShutdown.ps1 -ProbeConnections`: ready reached, exit 0,
  no new crash dump. Socket close messages from deliberate probes are expected.
  The existing waypoint-script 347 database warning is unrelated.
- Both servers were restored after validation. The final world startup reached
  ready at 15:41:42 local time; ports 1119, 8085 and 8086 are listening.

For DB2 reproduction, use the exports listed in the previous audit and add
`mechanics-link-hotfixes.tsv` from the hotfix database query below. This export is
required to reproduce the effective post-migration result; without it the tool
reports native/SQL references without deletion notifications.

```sql
SELECT Id, TableHash, RecordID, Deleted FROM hotfix_data
WHERE TableHash IN (1337593436, 2523372402) ORDER BY Id;
```

```powershell
python tools/research/audit_spell_mechanics.py --sql-directory .codex `
  --output .codex/resource-mechanics-fixed.json
./tools/tests/Test-CastResources.ps1
```

In-game visuals, interrupted casts, changing equipment effects and actual player
reconnect timing remain client acceptance checks. No client result is inferred
from a successful build, startup or isolated handler test.
