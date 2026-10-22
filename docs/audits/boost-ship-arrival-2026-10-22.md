# Boost tutorial: moving-ship arrival

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

At implementation review the new executable is staged, not yet deployed; Krau
has not yet been moved. Runtime backups and personal character recovery data
will remain in ignored local storage. Existing servers are user-owned; none
were stopped by the agent. A fresh process/port check found no worldserver or
bnetserver before deployment preparation.

Client confirmation is pending: cold login on deck without player/NPC falling,
movement after the hold, pet portrait, trainer and tutorial restart, logout/relog.
The complete one-opponent 1/1 -> two-opponent 2/2 -> Legion attackers -> boarded
exit bird/camera -> beach sequence, scaled one-hit surrender and pet/ally credit
are not client-verified by these arrival fixtures. Their combat, wave, credit and
departure handlers were not changed. Do not describe the full tutorial as
verified from compilation, data inspection or these isolated tests.
