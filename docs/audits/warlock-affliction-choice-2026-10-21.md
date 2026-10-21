# Calydus still offers acquired Ulthalesh, 2026-10-21

## Observed failure and cause

After the first artifact-choice fix, the client still displayed Affliction
alone. Read-only character queries confirmed rewarded acquisition quest 40623,
rewarded Demonology 42125 and Destruction 43254, but neither rewarded nor active
Affliction return quest 40712/41156. All three artifact entries were also
present in the character's item records. Item presence is supporting evidence,
not the completion gate.

The previous filter checked the return quests only. Native quest objectives
show acquisition quests 40623 and 41155 both require Ariden/claim credits
103473 and 100813. Completion of either acquisition already closes this weapon
route, even without a later return quest. This was an incomplete predicate in
the previous fix, not evidence that the player must obtain Ulthalesh again.

## Change

Migration 2026_10_21_02 adds twelve negative COMPLETE/REWARDED conditions for
40623 and 41155. They join the existing Affliction AND group in Calydus gossip
19326/0, choice 245 and response 523. Existing return-quest checks remain;
Demonology and Destruction retain their existing gates. All three acquired
means no artifact gossip option or choice window. Stale selection packets are
also rejected by the existing native response handler.

An incomplete or abandoned acquisition remains selectable. No item ownership
gate, quest grant/reset, inventory change or replacement mail is introduced.
The fix changes world conditions only and needs no executable rebuild.

## Verification

Test-WarlockArtifactChoice.ps1 passed using temporary SQL tables and extracted
production display/response handlers and condition admission logic. It checks
64 complete/rewarded route combinations across both acquisition and return
alternatives. A fixture with the reported quest history first removes the new
conditions to reproduce the Affliction-only offer, then restores them and
verifies no gossip/window and no stale-response side effects. Incomplete
acquisitions, initial selection, abandoned routes, class restrictions and
unrelated choices remain covered.

The temporary migration sequence applies the original migration, the new
migration, both again, and the new migration again. It finishes with 42 scoped
conditions and no changes outside that scope. Before applying the live
migration transaction, the scoped live conditions were backed up under ignored
.codex/backups/warlock-affliction-choice-20261021. The live database now has
42 artifact-choice conditions, including the twelve new acquisition gates.

With no user worldserver running, an owned hidden canonical Release server
(PID 30316) loaded the conditions and reached ready at 14:08:18. It received
`server shutdown 0` and exited 0 after database closure at 14:08:23. New startup
log lines contained no ERROR; DBErrors.log and scoped stderr were empty. No
validation worldserver remains running. Logs are under ignored
build-extractors/runtime-test/warlock-affliction-choice. The deployed executable
was unchanged by this SQL-only fix.

At the initial handoff, client reopening and disappearance remained pending. The
reported client failure supersedes any assumption that the earlier first fix
fully handled this character's acquisition history.

Subsequent client evidence: the user confirmed the artifact choice is gone and
provided an empty Calydus conversation screenshot. The separate remaining
**A Greater Arsenal** map marker is tracked in
[the map follow-up](warlock-artifact-map-2026-10-21.md); an empty gossip menu did
not verify map availability.
