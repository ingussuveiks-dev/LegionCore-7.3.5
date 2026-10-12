# Rocs vs Eagles (38910)

## Evidence and defects

TDB735 objective 279198 requires seven rescued Captive Great Eagles 94991,
storage index 0, flags 0/0. RewardNextQuest is 39321, The Three. Runtime
QuestLineXQuest places these quests at indices 2/3 in Skyhorn line 93.
The local quest links already match and are preserved.

Native Release Eagle 188432 has a dummy effect; Trapped Eagle 188434 is the
captivity aura. The existing click casts 188432 from the player and the
eagle's spell-hit handler starts timed list 9499100. Three defects existed:

1. The release event had neither a cooldown nor a non-repeatable flag.
   SmartScript::SetScript9 clears and replaces an existing timed list, so
   another hit could restart the credit/departure sequence and overwrite
   its invoker. The eagle remained clickable while waiting to depart.
2. The captivity aura was applied on reset and through the template addon,
   but never removed when rescued.
3. Timed action MOVE_Z had parameters 15,0,0. The actual handler interprets
   those as X,Y,Z offsets: it enabled flight but moved sideways at the same
   height. The corrected parameters 0,0,15 move upward by the same distance.

The migration makes the hit non-repeatable until reset, immediately removes
188434 and the spellclick flag through linked actions, and restores the
click flag alongside the existing aura on reset. Click and spell-hit guards
require the caster's quest to be incomplete. Invalid players therefore do
not consume a captive. Source-22 group 1 addresses event ID 0, as required
by ConditionMgr's eventId + 1 lookup. The existing timed list still grants
the invoker one credit, starts departure after one second and schedules
despawn five seconds later. No replacement spell is introduced.

## Availability and handoff

There are 13 unphased captive spawns in Darkfeather Valley, map 1220, area
7755, with 180-second respawn times. Their native spellclick and SmartAI
bindings are present. This remains a shared world rescue: after one player
releases a captive, another uses another captive or waits for respawn.

Lasan 93841 starts the quest. Lasan 95410 ends it and offers The Three;
the first Haglands spawn is GUID 340573 at 5246.3,3989.95,815.662.
Its phase IDs match the existing unconditional zone-7503 phase definition
23, so the handoff actor has a phase available to the player. The later
Lasan variants and their broader phase staging were not reconstructed in
this patch. Availability here is a database/core check, not client proof.

The [contemporary Blizzplanet walkthrough](https://warcraft.blizzplanet.com/rocs-vs-eagles-highmountain-quests-alliance/)
corroborates rescuing eagles and meeting Lasan in the Haglands, but describes
five eagles in a December 2015 build. The 7.3.5 archive's seven-objective
requirement is retained. The adjacent native JSON records archive/DB2
hashes, exact objective, ender, quest-line records and spell effects.

## Validation

- `Test-HighmountainRocsEaglesNative.py` reads the TDB735 archive and actual
  runtime DB2 files, comparing them with the saved evidence.
- `Test-HighmountainRocsEaglesData.ps1` applies the migration twice to
  temporary copies; checks both guards, linked aura/click cleanup and reset,
  timed credit/flight/despawn, all 13 spawns, turn-in and native objective;
  checks unrelated SmartAI/conditions remain intact. `-Live` is read-only.
- `Test-HighmountainRocsEagles.ps1` compiles extracted production SmartAI
  event/action entry gates, SPELLHIT handling, timer, reset and MOVE_Z code.
  It reproduces repeated legacy starts, tests wrong spell/invalid player,
  duplicate/concurrent hit suppression, original invoker retention,
  reuse after reset and actual X/Y/Z/flight flag changes. Action execution
  beyond the extracted entry gate uses a fixture boundary; SQL tests cover
  the configured linked actions and list.
- `Test-HighmountainBloodtotem.ps1` now exercises actual production credit,
  completion and prerequisite handlers for seven eagles, capped duplicate
  credits, wrong/absent/rewarded objectives and The Three reward gate.

Release compilation and startup/live SQL checks accompany deployment to
`build-extractors/bin/Release`. No boost, scenario, pet/mover or warlock
production code is changed. Client clicking, captivity visual removal,
flight animation and full playthrough still need in-game confirmation.
The next audit is The Three (39321) and its Haglands continuation.
