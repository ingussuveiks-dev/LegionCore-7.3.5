# Boost tutorial: moving-ship arrival

Current status: the final canonical Release is deployed. The user confirmed
visible ship, movement without falling and correct NPC feet on deck. The full
tutorial combat/departure sequence remains a separate client check. A recurring
silent instance-handshake failure interrupted one retest and recovered after a
client-only restart; its underlying cause remains unresolved (see the socket
authentication lifetime audit). Earlier failures below are retained evidence.

## Report and cause

The user reported Krau and nearby NPCs falling while the ship/world loaded on
login. Krau's saved row was offline on Horde tutorial map 1557, instance 1, at
Z -1.82825 with no transport attachment. This is below the tutorial ship.

The global object-floor arrival protection was still present. It covered ordinary
grid objects and static map-object transports, but explicitly exempted moving
transport passengers. Moving transports are outside the grid visitor and their
collision model can await its first dynamic-tree insertion. In addition, the
boost script attached the player only from its delayed Setup, after initial
self-create. Its timed forced passenger updates bypassed ordinary NPC visibility
deferral. These are code-path gaps; no log suppression or arbitrary flight aura
is used as a repair.

InstanceMap invokes OnPlayerEnter before Map::AddPlayerToMap/SendInitSelf.
MapInstanced creates instance transports before returning the map to admission.
The fix uses this ordering to attach and transform the native deck pose before
the client's first player object, without a second near teleport from Setup.

## Behavior

- The shared arrival builder queries actual phase-visible moving-transport
  collision models, including models not yet inserted in the dynamic tree, and
  sends supporting geometry before the player. No map/entry whitelist is added.
- An attached passenger retains the shared gravity/root hold while the same
  transport updates its world position. Disembarking, changing ships, entering
  a vehicle, death or map departure cancels it. Independent aura ownership is
  retained. Current-transport create is not duplicated after the geometry pass.
- New NPC/pet visibility and the boost script's forced passenger visibility wait
  for the same hold. A forced update retries instead of discarding its work.
- The existing 8-second post-loading notification grace and 30-second missing
  notification fallback remain bounded protection. The client provides no
  per-model collision-ready acknowledgement; this is not proof of renderer load.
- Missing tutorial transport remains an error with Setup retry. No fictitious
  floor is substituted. Normal instance construction provides the transport.

## Local data and regression evidence

Native runtime WorldSafeLocs.db2 and actual GameObjectModel collision queries:

| Faction | Map | Ship entry/display | Safe location | Local deck XYZ | Floor delta |
| --- | --- | --- | --- | --- | --- |
| Alliance | 1554 | 246606 / 34338 | 5219 | 11.1372, -0.363736, 20.6586 | -0.0156784 |
| Horde | 1557 | 246607 / 34366 | 5752 | 0.720768, 1.68496, 34.501 | +0.0000305 |

Test-BoostShipFloor.ps1 loads these DB2/model files through the native readers,
transforms the poses and rejects wrong-phase/disabled collision. No modern
TrinityCore data or external behavior assumptions were needed.

The new moving-ship MapObjectArrival fixture failed before the repair at
`sailor.selfCreateProtected`. The repaired extracted production handlers pass:

- Test-MapObjectArrival.ps1: moving floor pending dynamic-tree insertion,
  geometry-before-self exactly once, initial protection, NPC deferral, movement
  and ACK rejection, same-ship release, transport/vehicle changes, hidden/air
  destinations, existing ordinary/near/seamless/aura/timeout regressions.
- Test-BoostShipArrival.ps1: both factions, stale local offsets, existing or
  different transport, pre-world placement, missing transport and in-world
  recovery; guards prevent delayed Setup teleport/forced visibility bypass.
- Test-DalaranPortalLanding.ps1: native WMO and existing migration checks.
- Test-WarlockRegression.ps1 and Test-PetLoadSelection.ps1: existing mover and pet
  regressions. CreatureScalingTest.cpp: valid and invalid scaling ranges.
- Test-ScenarioResume.ps1 and Test-ScenarioResumeData.ps1: re-entry regression.
- Read-BoostScenario.ps1: matching native ScenarioStep, CriteriaTree and Criteria
  references resolve (14 scenarios, 186 steps, 536 trees, 116 criteria).
- Test-HandshakeDeadline.ps1 -VerifyReleaseBinary: existing socket lifetime
  regression retained in the staged executable.

The game and script libraries were rebuilt at their generated dependency paths.
Only the final worldserver link used a staging OutDir, with original final-link
library inputs. Linked-PE checks resolve incremental-link thunks and verify the
new model query, early attachment and forced visibility deferral call paths.
The old deployed binary fails the new model-query check; the staged binary
passes. Staged SHA-256:
`ef2e9d57aa87ab8e2130ca10e115913f2b1264c669dd2da4a6482ec283a54242`.

## Deployment and client status

Implementation commit `bcf6dfea` was pushed to origin/main. The executable and
matching map were copied to canonical `build-extractors/bin/Release`, and both
hashes matched staging. The deployed executable has the SHA-256 above and passed
the linked-PE arrival checks and Test-ScepterLogin.ps1 -VerifyReleaseBinary.
Configuration, data, certificates and DLLs were preserved.

Krau was confirmed offline. His complete character row and previous executable
and map are backed up under ignored local
`.codex/backups/boost-ship-arrival-20261022-104007/`. The recovery changes only
position XYZ, orientation and instance_id: map 1557/zone 8422, instance 0,
(-2556,2939.6,149.4), facing 1.98. This is CharacterService's native Horde boost
bootstrap; OnPlayerEnter computes the current ship transform from safe location
5752 before self-create. A complete before/after row comparison confirmed all
other character fields unchanged. No quest, level, inventory or pet reset was
performed. The initial strict floating-point equality guard matched zero rows;
a bounded range for the observed old Z matched exactly one offline character.

Existing servers were user-owned; none were stopped by the agent. A fresh
process/port check found neither server before deployment. The agent subsequently
launched only hidden worldserver PID 24280 for canonical runtime validation.
It reached ready, exchanged native greetings/initializers/challenges on both
8085/8086, and retained the expected timeout for a deliberately silent test peer.
It was shut down with `server shutdown 0`, exited 0, and closed its database
pools. No agent-owned server remains. DBErrors.log remained empty; this
startup is not a tutorial traversal.

The user subsequently launched visible bnetserver PID 33088 and worldserver PID
30304. The latter reached ready at 10:44:12. Logs record Krau login at 10:44:57,
tutorial scenario 1083 starting on map 1557, and passenger visibility at 10:45:07.
There is no missing-transport error in this admission. These server observations
do not establish whether the client displayed falling or whether movement works.

Client confirmation is pending: cold login on deck without player/NPC falling,
movement after the hold, pet portrait, trainer and tutorial restart, logout/relog.
The complete one-opponent 1/1 -> two-opponent 2/2 -> Legion attackers -> boarded
exit bird/camera -> beach sequence, scaled one-hit surrender and pet/ally credit
are not client-verified by these arrival fixtures. Their combat, wave, credit and
departure handlers were not changed. Do not describe the full tutorial as
verified from compilation, data inspection or these isolated tests.

## First client failure: ship disappears after create

The user reported that Krau loaded without falling, but the ship did not appear.
This invalidates the initial arrival handoff as a complete client repair. The
initial automated fixtures stopped after object creation and hold release; they
did not execute VisibleNotifier::SendToSelf's subsequent out-of-range cleanup.

Root cause: object-first arrival now records the ship's create in m_clientGUIDs.
Moving transports are not ordinary grid objects, and VisibleNotifier protects
their passengers but did not protect the ship itself. Its unvisited GUID thus
became an out-of-range removal immediately after creation. This is a regression
exposed by the new tracking; keeping the create untracked would conceal the
lifecycle gap rather than reconcile visibility correctly.

Test-TransportVisibility.ps1 extracts and executes the complete production
SendToSelf. Before the follow-up repair it failed at the first retained-ship
assertion. The repaired cleanup retains only an actual live map transport that
the player can see. Both aboard and nearby observers retain it across repeated
cycles; missing, unloaded, hidden and wrong-map transports are still removed.
Ordinary object removal, existing passenger retention and vehicle extra-look
exceptions also pass. No visibility or error check was disabled.

The game library was rebuilt at its original link-input path, then worldserver
was linked to staging with original libraries. The staged follow-up SHA-256 is
`710e507dd9b10121e190aab14a48a9a279de70fe6c94a6222c03f127975338f5`.
MapObjectArrival and BoostShipArrival regressions pass again; the linked-PE
verifier additionally requires SendToSelf -> Map::GetTransport. The user-owned
server is still running the initial build at this review point. Follow-up
deployment and visible-ship client confirmation are pending.

Computer Use was attempted only for read-only game-window inspection. Its native
pipe was unavailable, including the documented retry/reset recovery; no UI
actions were taken and no game-window visual claim is made from that tool.

Follow-up implementation `7a876754` was pushed to origin/main. After the user
confirmed graceful worldserver shutdown, a process check confirmed it was gone.
Only the executable and matching map were replaced in canonical Release, with
matching staging/deployment hashes (follow-up SHA above). Previous binaries are
in ignored `.codex/backups/boost-ship-visibility-20261022-105524/`.
Canonical linked-PE arrival/visibility checks, Scepter login and handshake
lifetime regressions pass. No second hidden validation server was launched.
The saved Krau row now contains the correct native ship-local deck offsets;
no further character data write was needed for the visibility deployment.
The user has been asked to retest actual ship display and movement.

## Second client result: arrival verified, NPC deck height incorrect

The user confirmed: the ship is visible, movement works and Krau no longer falls.
They additionally observed NPC feet inside the deck, except the nearest actor.
This verifies the reported arrival behavior in the client, not the whole tutorial.

Native collision probes at the script's actual local offsets explain the height
failure. Every standing NPC reused the safe location's Z, although the deck is
not flat. Horde trainer (5,0) is only 0.043 yards below its surface; entourage
(8,-6/-2/2/6) are 0.313/0.256/0.305/0.362 below. The exit actor at (12,0) would
be 0.504 below. Alliance offsets also differ (including -0.194 and +0.139).
These are real model ray results, not an arbitrary visual-height adjustment.

The scoped script helper now transforms each grounded passenger pose to world,
queries only its actual ship model, and converts the supported height back to
transport-local coordinates before native summon/home initialization. The
wrapper restores that resolved pose after passenger-set handoff. Missing model
or floor logs an error and rejects the placement; it does not invent support.
An explicit airborne mode preserves the Legion bat's intentional +3 height.
Player safe locations, movement/credit/wave/departure logic remain unchanged.

Test-BoostShipFloor.ps1 now executes the extracted production placement helper
and Offset with actual native Transport instances/transforms, runtime DB2 and
collision models. It covers both factions' trainer, entourage, dummy, both
sparring positions, all ten imps, infernal and exit poses; every final world Z
matches its own model floor within 0.001 yards. Repeated projection is stable,
X/Y stay fixed, intentional flight stays airborne, missing/disabled floor fails.
A naive shared-Z negative control compiles but fails the real floor-contact
check (exit 9). The corrected native test passes. BoostShipArrival and full
TransportVisibility tests pass again. The linked-PE verifier checks native
model queries in the helper and its inlined SummonPassenger implementation;
the first check expected a non-inlined call and was corrected for MSVC inlining.

Original-path script libraries were rebuilt and a final staging link completed.
Staged NPC placement SHA-256:
`5ab1cf6f94bfc861ac298e27267f3fd20757741c7111a3c9052ece8d310a39be`.
Deployment and client confirmation of NPC feet, later combat movement and the
full training/departure sequence remain pending at this implementation review.

NPC placement implementation `8d44973e` was pushed. Following the user's next
shutdown confirmation and a no-worldserver process check, its executable and
matching map were copied to canonical Release. Both copied hashes match staging
(NPC placement SHA above). Prior binaries are preserved in ignored
`.codex/backups/boost-ship-deck-20261022-111314/`. Canonical BoostShipArrival native
call-path checks and Scepter login verification pass. No hidden server or
character-data write was used for this deployment. Client retest was requested.

## Final client arrival evidence

The first retest was blocked before player loading by the separate instance
handshake failure recorded in `socket-authentication-lifetime-2026-10-22.md`.
After restarting only WoW, with the same servers left running, the user confirmed
successful Krau login and correct NPC feet on the deck. Earlier in this run they
had already confirmed persistent visible ship, working movement and no falling.

Server evidence: user-owned bnetserver PID 9568 and worldserver PID 30752 started
at 11:13:45/11:13:48 and remained unchanged. Fresh client PID 32808 started at
11:26:16. Instance peer 54301 completed continued auth and encryption, Krau login
and scenario 1083 began at 11:26:31, and Client.log records Active Player Created
at 11:26:31.362. NPC visibility followed at 11:26:40; no supporting-deck error was
reported in this admission. These logs support the observed admission, while
the user's report supplies the actual visual confirmation.

The user explicitly confirmed they are not continuing the tutorial yet and have
tested arrival only. The full one-opponent/two-opponent/Legion/exit flight
sequence, later combat movement,
pet/ally killing blows, camera, pet portrait and beach dismount remain unverified
in this session. No agent-owned hidden server remains. The visible user-owned
servers are left running, and no further character data changes were made.

## Later tutorial report (2026-10-22)

The user subsequently began Krau's tutorial. Arrival/feet remained correct, but
the hunter lesson "Pet, Attack!" is blocked at 0/3: Kill Command reports "You
can't do that yet". Client diagnostics returned `KC false pet true` and UI error
ID 50. Further requested evidence is action slots 1..6, IsSpellKnown(34026),
GetSpellCooldown(34026) and player focus; a chat screenshot is sufficient.
Do not infer a missing pet from this report or relax the pet-control/cast checks
without the actual failing state. No boost-sequence completion is verified.

The separate Highmountain quest/mover repairs were built and deployed with both
user servers already stopped. The owned hidden validation server reached ready
and was gracefully shut down. This supersedes the earlier process-status note;
no agent server is left running. These repairs do not yet resolve this hunter
ability report or the intermittent socket initialization failure.
