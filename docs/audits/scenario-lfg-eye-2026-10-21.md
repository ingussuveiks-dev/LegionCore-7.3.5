# Persistent scenario minimap eye, 2026-10-21

Maiko reported that the minimap eye had continuously shown `Scenarios / In
Progress` since the level-100 boost, including after reaching Karazhan. This
report is retained as client evidence; no complete client replay has yet been
performed for this fix.

Read-only database evidence before the change: Maiko (GUID 1) was saved on
map 0, zone 41, instance 0, with an empty scenarioResume. Quest 40518 was in
character_queststatus_rewarded. Group 1 still had one member, groupType 12,
and lfg_data `(dungeon=908,state=3)`. State 3 is PROPOSAL in the current native
enum, not a completed run. _LoadFromDB restores its slot but does not restore
that transient state. These are persisted facts, not a read of live server
memory. No direct character, quest, item or group repair SQL was applied.

## Route and defects

1. CharacterService puts ordinary boosted classes on maps 1554/1557. The
   boost instance constructs its class-specific scenario directly with
   AddScenario(instance, scenarioId); it does not JoinLfg or create an LFG
   group. The class lessons, sparring waves, boarding and flight code are
   unchanged by this fix.
2. The departure bird casts 219912 on passenger 0. The existing Broken Islands
   handoff waits for the exit spline, then joins slot 908. That is the first
   LFG group in this route. Failed-queue recovery and pet/mover handling remain
   unchanged.
3. Native runtime DB2 confirms LFG 908 -> map 1460 -> scenario 786, type 4,
   scenario queue subtype and difficulty 12. Scenario::Reward excluded type 4
   from FinishDungeon. It now uses the same matching-dungeon guard as other
   scenarios; an unrelated old LFG group cannot be rewarded by another run.
4. FinishDungeon, LeaveLfg and RemoveGroupData changed player state before
   fetching selected slots for the client update. Native SetState clears
   those slots for NONE/FINISHED. SendUpdateStatus then selected queue/ticket
   0. The slots are now copied before clearing, with the actual group slot
   used for an already-cleared finished selection.
5. Completion was not persisted after the initial group creation. It now
   saves FINISHED, so restart cannot restore the initial active state.
6. SetupGroupMember did not restore queue registration, LFG group association
   or a valid ticket after restart. Consequently HasQueue could suppress the
   removal update. Registration and missing tickets are restored; existing
   tickets retain their timestamp. Ordinary parties are excluded.
7. A completed solo scenario group was left attached after exiting. Generic
   login/map-change cleanup now disbands it only outside its own map. Native
   teardown sends removal before dropping queue registration, deletes group
   persistence and restores timewalking state. Unfinished return groups,
   multiplayer groups and ordinary dungeons remain available.
8. Legacy Broken Shore groups such as Maiko's need recovery even when their
   saved state was never FINISHED. The intro login/map-change/reward hooks
   recognize the faction's already rewarded 42740/40518 and its actual map-1460
   LFG group, then use the same native teardown. Hook ordering is covered by
   registering the restored member before disbanding.

Later Warlock routing is confirmed with the native C++ DB2 loader: New Blood
1183/1522/962, Ritual Ruination 1184/1522/998, Dark Whispers 1250/1498/1097,
and Dark Riders 1185/1533/988. Their matching completed LFG groups use the
generic exit path; direct scenarios without an LFG group do nothing. The
Scepter's scenario selector/instance recovery remains unchanged. Map 1600's
base LFG entry has no ScenarioID and uses content-specific scenario selection;
the tests do not mistake that base entry for the selected live scenario.

## Automated verification

- Test-ScenarioLfgLifecycle.ps1 executes extracted native player/group state
  setters, completion bookkeeping, Scenario::Reward type dispatch, queue packet
  construction, leave/disband, restored registration/tickets and legacy intro
  cleanup. Covers both factions, incomplete exits, completed exits, staying in
  the completed map, restart/login hook ordering, repeated cleanup, other
  queues, ordinary/multiplayer/dungeon boundaries and later scenario maps.
- ScenarioLfgCatalog.cpp reads the runtime DB2 through the core's own loader,
  including scenario subtype/type/difficulty and the departure spell effects.
- Test-BrokenShoreReentry.ps1, Test-ScenarioResume.ps1,
  Test-ScenarioResumeData.ps1, Test-NewBloodLogin.ps1 and Test-ScepterLogin.ps1
  passed. SQL fixtures use connection-local temporary tables.
- Release game/scripts libraries built at their original generated dependency
  paths. The validation worldserver link uses those exact inputs with project
  reference building disabled; no global static-library OutDir override.
- verify_scenario_lfg_release.py checks ten calls in the actual linked x64 PE:
  completion/persistence, generic login/exit, three intro hooks, restored member
  setup and native Disband. It resolves incremental-link thunks and bounds
  ordinary functions with native unwind metadata; tiny leaf wrappers use the
  next public symbol. A string marker alone is not treated as proof.

## Remaining client checks

On Maiko's next login, confirm the legacy group is removed and the eye goes
away. Replay a new boost through departure/landing and both faction exits,
confirm it is present only while participating, and check later artifact
scenario exits, relog/restart recovery, unfinished return, camera, movement
and pet portrait. Compilation, native fixtures and server startup do not
verify those visible interactions or full quest completion.

## Deployment and startup

The validated worldserver.exe and its matching MAP were copied to the canonical
build-extractors/bin/Release directory while both user servers were stopped.
The previous executable/MAP were backed up under the ignored .codex/backups
directory. Configurations, certificates, DLLs, extracted data and log directories
were preserved; bnetserver.exe needed no change.

Canonical executable SHA-256:
`fb2c2a90679c104ed661c38b9b6fe9098f7e7f609208d146a6d14229805718e7`.
Test-ScenarioLfgLifecycle.ps1 -VerifyReleaseBinary and
Test-ScepterLogin.ps1 -VerifyReleaseBinary both passed against this deployed EXE.

An owned hidden validation worldserver (PID 13532) reached ready, with zero
Server.log ERROR lines and an empty DBErrors.log. It was shut down with the
native `server shutdown 0` command and exited with code 0. No validation
server was left running. No client logged in during this startup test, so it
does not prove that Maiko's old group was removed or that the eye disappeared.
