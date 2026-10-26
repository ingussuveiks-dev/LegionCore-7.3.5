# Artifact repair continuation checkpoint

Written after `c1781cbc` on 2026-10-26. Read this together with **AGENTS.md**, current Git status/log, and the worklist; it is a checkpoint, not proof that the whole project is finished.


## Latest update after this checkpoint: Heroic Leap

- User showed the next Warrior1093 step8 **Jump Into The Fray**, Heroic Leap6544 still returning No path available on the ship. This supersedes the earlier immediate next task until the client retest is reported.
- Repaired `CheckElevation` with real transport-model landing/footprint/arc body clearance using live94954 effect1; retains ordinary unsupported-deck terrain fallback, root/BG/water/falling and native movement/criteria. No fake credit/character edits. See `docs/audits/heroic-leap-transport-2026-10-26.json` for actual sources, initial development failures, native limits and checks.
- `Test-BoostHeroicLeap.ps1 -Baseline/-AddressSanitizer`, actual both-ship `Test-BoostShipFloor`, typed26972/readonly hotfix/lesson catalog, Charge/tutorial overlay/shared construct/arrival/action persistence pass. Canonical selected Leap/Charge PE and Scepter Release pass. **Client leap/landing/1-of-1 and later full boost remain pending.**
- Promoted while both user servers absent; own33700 fresh ready/shutdown0/exit0, stderr/newDBErrors0, protected hashes unchanged, no task-owned server remains.
- Current canonical world SHA256 **34EFE53A680A99315D7FADE7E7E00996CD6467F6DE210C147CEE51CB22E45189**, world.map **762CA74844F69F0D7CBC27CB4648C3148530304680BE3C2BDB438A9CFA4167F1**, bnet **4DE8946B5729FA612E9C447E8B537CA1D4AE7AF21ED8D13515CD3609FB3E4CC1**. These supersede the older deployed hashes below.
- The formerly deferred232-byte obsolete creation-spell log is now **resolved and archived offline**, exact hash9EB36EF23920FB914322C2CE213B3491F9B8A884754EF2E3768FE799E105C716, under ignored `build-extractors/bin/Release/fixed/warrior-creation-actions-2026-10-26/`. Existing cause fixbfdf3354/current deployed data and native regression verified; exclusive file/process/hash rechecks before zeroing. Active **DBErrors0/Crashes0** at this update. The old deferred section below is historical, not current maintenance work.
- Recheck processes/logs before doing anything: user can launch again. Hunter additional-choice patch is still ignored, untested/undeployed and OPEN; it was not included in this Leap build. Resume that independent work after the focused repair; keep the new chat's instructions and worklist.

## User authorization and current testing

- Continue actual investigation/repairs for all36 Legion7.3.5.26972 artifact acquisition chains and effects. Each focused repair needs relevant regression, honest audit, commit and push of the current branch to origin, without force or runtime/credentials/character data.
- User already acquired/upgraded Hunter BM Titanstrike and previously tested/acquired the Warlock weapons. They are now testing **Orc Warrior Arms**, starting with level100 boost/ship lessons, then Arms, Fury and Protection artifacts. All three Warrior routes have repairs, but **are not certified complete**. Preserve remaining findings in the worklist.
- User may be away. Continue independent code/data/native/TEMP-table tests and separate builds without waiting for them. Do not stall all work on deployment. Do not stop their servers or mutate/reset their characters/quests/inventory/auras to hide a broken chain.
- Promote a tested build only when no user/unknown worldserver or bnetserver is running. If running, keep the canonical EXEs/logs intact, retain deployment/archive as deferred and work on another independent issue. User explicitly permits promotion when their servers are absent; do not request repeat permission in that case.
- Last observed user/unknown processes: bnet30528/world22404, canonical paths. **Recheck**; these are not task-owned. No task-owned hidden server remains. Do not kill processes based on this snapshot.

## First steps

1. `git status --short`, `git log`, read AGENTS.md. Expected tracked worktree clean at this checkpoint; do not discard new changes.
2. Read `docs/audits/artifact-acquisition-worklist-2026-10-23.json`, `artifact-routes-2026-10-23.json` and `legion-artifact-donor-review-2026-10-23.json`. Preserve earlier results rather than restart36 routes from scratch.
3. Recheck actual processes and canonical diagnostics. Before/after **every** test/build/runtime, use `tools/tests/Invoke-WithRuntimeDiagnostics.ps1` with actual fixture runtime directories. Passing assertions/ready do not override a crash or unexplained DB/server error.
4. Resume the unfinished Hunter additional-selection investigation below while the user tests Warrior. If user provides a new failure, incorporate it into active work, preserving existing tasks.

## Recently completed and deployed Warrior boost repairs

Commits, already pushed to origin/main:

- `c734844c`: Charge100/Intercept198304 on a common moving Transport. Both outer Warrior cast check and native charge-effect admission now call target-aware PathGenerator. A shared corridor checks the real ship collision model: continuous floor, finite height/step, body LOS, lateral caster-radius support, phases, enabled collision and point budget. Rejects invalid corridors; no forced success/ignored pathfinding, Rage grants or fake scenario credit. Ordinary/different-transport paths, root/range/LOS/water checks and native contact shortening/spline conversion retained.
- `bfdf3354`: remove22 exact obsolete Warrior creation defaults78/button1 and88161/button73/type0, absent in matching Spell.db2/hotfix. Keep validation diagnostics, real6603, custom positions/nonspell actions and saved character bars. Migration11 is actually RELEASED and obsolete-default count0.
- `79f1451c`: one instance-owned action overlay refresh after real client loading/arrival protection ends, scenario exists and player is still on the actual gunship. User saw full bars on first boost entry, but closing/reopening the client showed the correct empty intro bar and then only Charge for its lesson. The exact client overwrite event was not captured; late initialization is an inference. Saved full layouts remain intact and reappear outside ship maps. Do not clear character actions to achieve this.
- `c1781cbc`: combined deployment/diagnostic handoff.

Canonical `build-extractors/bin/Release`:

```
worldserver.exe SHA256 74ABDBCFE820996FA31F7409A385F2603D33820201ECCBE241D8111F7BE5BF9F
worldserver.map SHA256 D8198809358C1F954FCED1DCA8D9AFE8AA0854A3E38A75FDF2A56E5CEB4FA07C
bnetserver.exe  SHA256 B2EB3A39C5E5C59A9F3BCA3303C4364FBA9AC3F5CD16E646AAF0852EEE6E0336
migration11 SHA1      70BFFCADDD2DD51C84873C2BF9E7A7198F33F80C
```

Actual original game/scripts/shared/common LIB inputs verified; no global OutDir override. Own8564 reached fresh ready and shutdown0/exit0, stderr/newDBErrors0, protected runtime hashes unchanged. All final test inventories had no new crashes/errors. Last Crashes count0. Native tests do **not** prove real client Charge/lesson credit/camera/pet/UI/island traversal; user was asked to retest Charge and had not yet reported the result.

Audits: `transport-charge-ground-path-2026-10-26.json`, `warrior-creation-actions-2026-10-26.json`, `boost-tutorial-late-action-overlay-2026-10-26.json`, `boost-warrior-deployment-2026-10-26.json`.

Relevant tests: `Test-BoostCharge.ps1 -Baseline/-AddressSanitizer`, `Test-BoostShipFloor.ps1` (real models for both ships), `Test-BoostTutorialBars.ps1 -AddressSanitizer`, `Test-WarriorCreationActions.ps1` (TEMP + native typed catalog), `Test-BoostConstructLessons.ps1`, `Test-BoostShipArrival.ps1`, `Test-BoostActionBars.ps1`, `Test-ActionBarPersistence.ps1`, and selected `verify_transport_charge_release.py`/`verify_boost_tutorial_bars_release.py`/shared boost PE verifiers. Canonical Scepter login Release regression also passed. Existing fixture/build warnings documented, not hidden.

Separate open observation: generic `PathGenerator::GetTotalLength` omits its final segment. Not repaired or claimed resolved by the corridor change. Preserve this finding if addressed later.

## Deferred old DBErrors archive — do not clear while user server runs

At checkpoint `logs/DBErrors.log` still has **232 bytes**, exactly the two old missing-spell78/88161 creation messages. SHA256 from read-only shared access:

`9EB36EF23920FB914322C2CE213B3491F9B8A884754EF2E3768FE799E105C716`

The content cause is repaired by `bfdf3354`, tested and loaded by updater11. An attempted offline archive refused **before any mutation** because the user restarted both servers after being told the EXE was ready. **No archive/truncation occurred.** This is correctly deferred maintenance, not a new runtime failure. When safely offline, recheck bytes/new entries immediately, archive only wholly resolved evidence under ignored `build-extractors/bin/Release/fixed/warrior-creation-actions-2026-10-26/`, preserve/verify hashes and then zero the reviewed active log. If new/mixed evidence appears, investigate it and leave unresolved material visible. Ordinary Get-FileHash sharing can fail on the writer-held log; read-only FileStream with FileShare.ReadWrite worked for checksum only. Never bypass the active-server maintenance rule.

## Unfinished Hunter additional-artifact selection — next main repair

Tracked audit: `hunter-additional-artifact-offer-open-2026-10-26.json`; worklist marks OPEN. The reported Hunter is level105, has rewarded40618 and BM chain41541/41574/42158/42185 plus chosen40621 and Continuing the Legend44043, but no Survival/Marksmanship chosen/start. Do not export identifiers or grant missing selections to that character.

Proven content gap:

- Trueshot Emmarel107317 uses menu20227, has npcflag2 (questgiver only), no SmartAI rows and no selector interaction.
- Later Emmarel107973 uses the **same empty menu20227**, npcflag3; preserve her original SmartAI id0 reward42659 -> invoker216477. She has multiple phase/location variants; do not repair only the first actor.
- 107317 phase5939 admits rewarded42519 before42395. Later107973 phases5953/5955 follow42395/42399. No phasing repair has yet been justified: menu/flag/interaction gap is proven.
- Native44043 localized text asks to choose a second artifact, min level102, current zero objectives/RewardSpell/StartScript/CompleteScript. The previous exact user interaction that rewarded it without a second choice was not captured; don't invent history or reset it.
- Dalaran Emmarel102478 menu19115 option0 already has review-weapons broadcast103645 and native SmartAI cast198430 (choice240), gated by rewarded40618. Phase6764 admits active/complete/rewarded40384. Alternative not traversed in the current client.
- Native choice240 responses504/505/506 and spells198433/198435/198436 record chosen40621/40619/40620; existing `class_hall_artifact_onboarding.cpp` then starts41541/41542/41540 via ordinary admission, never grants weapons.
- Actual acquisition finale quests to examine for finished-route filtering: BM42185 Never Hunt Alone; Survival40385 The Spear in the Shadow; MM40419 Rescue Mission. **Do not hide an abandoned unfinished route just because its hidden chosen quest persists.** Preserve first selection, retry/re-entry, completed-route exclusion and stale response execution checks.

Partial, **untested and undeployed** code was isolated when the Warrior reports arrived:

`.codex/hunter-additional-choice-in-progress.patch` (2739 bytes at checkpoint; ignored local file).

It extends the existing Warlock245 condition consumers in `Player::SendDisplayPlayerChoice` and `WorldSession::HandleChoiceResponse` to240 and avoids an empty Hunter choice frame. No matching Hunter condition/menu migration or tests have been implemented. It was reversed from the tracked tree before the Warrior build; current canonical EXE **does not include it**. Inspect/rebase it against current HEAD before reusing; **do not just deploy it**.

Possible focused implementation: restore an appropriately admitted native review/accept interaction for both Trueshot Emmarels, keep later campaign rows, native198430/UI240 and ordinary onboarding. Add and exercise matching source15/32/33 data, first-versus-additional level/progression admission, real acquisition completion filters and stale response rechecks. The existing Warlock choice native/TEMP tests are references, not Hunter test evidence. Native Condition14 is QUEST_NONE,27 is LEVEL (comparison3 >=); **47 is TERRAIN_SWAP, not a modern quest-state condition**. Read this version's actual enums/loader.

Keep all unfinished code-coupled SQL outside live updater directories until matching executable promotion, even if uncommitted: user restarts can auto-apply it. Test custom-row refusal/idempotence and both actor variants. Client selection/acceptance/full MM+Survival remain separate checks.

## Other inherited work and evidence

- Hunter BM acquired/upgraded by user; general routes still have unresolved lifecycle/VFX edges. Allari Scythe37660 phase/exit, Faronaar personal companion and optional Stellagosa return were repaired earlier; see their audits/AGENTS. No reported client success yet for the last Stellagosa auto-board fix.
- Dreadblades Outlaw pickup/Eliza death were repaired and deployed before this checkpoint. Next prior code investigation was Degauz102185: pre-hit90/60/30 checks are Player-only and allow scaled one-hit death to skip surrender. No edit/test fix yet. Need real post-scaling health convention/owner resolution/stage0 eligibility and original retreat/dialogue, not a fake artifact grant. `DamageTakenAfterScaling` exists; do not compare post-scaled damage with template health.
- Warrior Arms/Fury/Protection have many focused repairs but full return/visual/client sequences remain unfinished. Do not describe them, Warlock or all36 as completely verified because the user was advised to test them.

## Local tooling

- `.codex/silver-hand-query.ps1 -Query ...`: read-only World DB using private runtime configuration. Inspect schema before using columns; helper buffers output and an invalid later SQL statement can discard prior results.
- `.codex/scythe-exit-readonly.ps1 -Query ...`: read-only Character DB. Prefer needed anonymous aggregates/content checks; do not mutate characters or commit dumps.
- `tools/research/Read-LegionLegendaryCatalog.ps1 -SourceFile Name.cpp -ProbeName Name -OutputFile ...` uses the actual typed reader; SourceFile is relative to tools/research. `LEGION_DIALOGUE_CONFIG` may point to the canonical config path; never print its credentials.
- Build original generated `build-extractors/src/server/scripts/scripts.vcxproj` Release/x64 (depends on game), then `.codex/link-staged-release.ps1` for separate final links with original LIB inputs. Verify staged implementation, not only matching file hashes.
- `.codex/deploy-active-grid-release.ps1 -Issue unique-name` refuses active servers and preserves protected runtime files. `.codex/silver-hand-presentation-startup.ps1 -EvidencePrefix unique-name` owns and gracefully shuts down its fresh validation worldserver; never run against an already active user server.
- Trusted Python: `C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe`; PE dependencies via temporary `PYTHONPATH=.codex/pe-review-deps`. Restore environment values after tools. No Defender exclusions/quarantine bypasses.

No subagents were authorized. No goal/automation or background continuation is created by this checkpoint itself: the new conversation must receive the user's continue instruction.
