# Calydus completed artifact choices, 2026-10-21

## Observed client failure and cause

Maiko's client screenshot shows Calydus offering "Let me select my artifact"
and all three weapon responses after all three acquisition chains. The same
screenshot confirms A Mutual Friend was completed and Rise, Champions accepted;
Champion: Calydus is a legitimate, independent quest offer.

Menu 19326 option 0 checked only rewarded introduction 40684. The native choice
display's Source-32/33 condition calls were commented out, and the response
handler executed the hidden chosen-quest reward spell without these checks.

## Change and boundaries

Migration 2026_10_21_01 adds 30 scoped conditions. A route is finished when its
final quest is COMPLETE (awaiting turn-in) or REWARDED:

- Response 523 / Affliction: 40712 or alternate ending 41156.
- Response 524 / Demonology: 42125.
- Response 525 / Destruction: 43254.

Source-15 gossip retains rewarded introduction 40684 and requires at least one
unfinished route. Source 32 admits Warlocks with an unfinished route. Source 33
filters each completed response separately. All three finished means no gossip
option and no reopened choice window. Native display and response execution
now evaluate these conditions for choice 245; stale responses cannot cast the
reward spell or invoke onboarding. Other choice IDs retain their behavior.

Hidden Chosen quests do not close a route: abandonment/retry remains available.
Item ownership is deliberately not a completion gate, so a shop/bank artifact
does not replace completing its story. The initial 40684 accept-triggered
window still works before that introduction is rewarded. Champion recruitment,
other Calydus options and quest rewards are unchanged. No character state is
reset or granted. The user chose deletion prevention rather than automatic
replacement or a replay/reset system; see the separate deletion audit.

## Checks

- Test-WarlockArtifactChoice.ps1 applies the migration twice to temporary
  condition tables, checks 30 rows and preservation outside the scoped offer,
  then compiles actual SendDisplayPlayerChoice and HandleChoiceResponse.
  Actual condition cases, negation and AND/OR dispatcher execute 32 combinations
  of complete/rewarded endings, including both Affliction endings. Checks cover
  exact displayed responses, gossip, stale response side effects, class gate,
  first selection, hidden Chosen retry and unrelated choice behavior.
- Test-ArtifactOnboarding.ps1 passed all 36 existing routes, 900 native race/class
  and ownership cases, and 2431 quest admission cases.
- Test-ScepterLogin.ps1 -VerifyReleaseBinary passed. The PE/unwind/thunk check
  verifies both actual deployed choice handlers call the native condition
  lookup, alongside the ten existing scenario-LFG call paths.

The world conditions were backed up under ignored .codex/backups before the
transactional live migration. Release game.lib was built at its original final
link path, then worldserver linked with BuildProjectReferences=false and only
the executable OutDir changed. Tested EXE/MAP were copied to the canonical
build-extractors/bin/Release while no worldserver was running; other runtime
files were preserved. Combined executable SHA-256 (also includes the separate
artifact deletion guard):
`b42670b264b30ca862459280311aa8e7f40433d129d8607ea7ac105cc6317d57`.

Owned hidden worldserver PID 7676 reached ready at 13:36:54, received
`server shutdown 0`, and exited 0 after database closure at 13:36:59. Its scoped
new Server.log lines contained no ERROR; DBErrors.log and stderr were empty.
No validation server remains running. An earlier validation wrapper encountered
a log-sharing exception; its finally block still shut that owned process down
gracefully. The corrected shared-read wrapper completed the check above.
Logs are in runtime-test/warlock-artifact-choice. Native checks and startup do
not verify actual client rendering: reopening Calydus with all three finished,
partial acquisition, the initial selection and Champion: Calydus remain client
checks. Preserve this distinction in subsequent fixes.
