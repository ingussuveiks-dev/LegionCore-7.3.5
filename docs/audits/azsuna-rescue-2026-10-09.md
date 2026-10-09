# Azsuna rescue audit — 2026-10-09

Scope: Save Yourself (37530), then The Head of the Snake (37470). These were explicitly excluded from the preceding Nar'thalas academy repair.

## Findings and repair

37530 had seven identical quest-start command-7 rows. These completed the quest without its seven native objectives and made the loader restore the exploration/event requirement. The new migration removes only those shortcuts and that event bit; objective IDs and storage slots remain unchanged.

Farondis 89009 now offers a personal replay. Boarding waits for the teleport acknowledgement, uses native vehicle 3972 / controlling seat 15240 and aura 178284, and credits speaking only after the correct player boards. The native Fireball 178784, Meteor Storm 179215 and Blink 225948 populate the previously empty vehicle bar. Meteor's dummy now dispatches native ground-area damage 179217. This pairing is an inference from native spell names, compatible target layouts and the original creature spell list; it is not presented as a recovered retail server script. Native level damage calculation is retained (the small DB2 base damage is not final damage).

The replay requires academy arrival, pursuit arrival, two distinct personal naga defeats, cave arrival, the Azshara encounter, and reaching the personal prisoner. Location checks include vertical distance. Death, abandonment, logout, map departure, timeout and dismount clean up the personal actors. Completed quest objectives survive a retry. The existing Farondis questender remains available for recovery after completing the replay and disconnecting before turn-in.

Azshara's vision cannot die to an early one-shot. An offensive Farondis spell starts native spell 197936 / scene 1148 / package 1520. SceneScriptText and its Path 15600 provide the actual cave location and 20-second defeat / 22-second ending. An early scene completion/cancellation callback aborts the attempt; it never grants progress. The event also requires the player to remain aboard and at the cave. The native controlling seat supplies ordinary charm/vehicle cleanup; no shared mover, pet or boost code was changed.

37470 already has Athissa 88855, its ordinary kill objective, questgiver, questender and next-quest link. Its reward spell 181404 already plays conversation 346. An additional identical SmartAI reward cast was removed. The boss's general combat behavior is unchanged.

## Evidence and limits

- Native build 26972 DB2, sparse SceneScriptText, game tables and TDB 7.3.5 objective archive; IDs/hashes in `azsuna-rescue-native-2026-10-09.json`.
- Existing world quest POIs and nearby spawn heights supply academy/pursuit locations. The two replay enemies use the pursuit location; this is not an original sniffed spawn arrangement.
- [Wowhead: Save Yourself](https://www.wowhead.com/quest=37530/save-yourself) corroborates controlling Farondis, the three abilities and the seven objectives.
- [Wowhead: The Head of the Snake](https://www.wowhead.com/quest=37470/the-head-of-the-snake) corroborates the final Athissa kill.

This restores a server-side playable progression, not the complete retail choreography. The replay naga use actual combat deaths instead of their scripted escape; dragging actors and introductory dialogue are not reconstructed. Meteor Storm currently produces one native area impact per button cast, rather than a reconstructed multi-impact storm. The native Azshara scene is retained, but its rendering, camera, actor smooth-phasing, actual combat balance, pet portrait and final turn-in still require a 7.3.5 client playthrough. No such client test was performed. In particular, unit tests cannot establish that native vehicle movement and its action bar work in the client.

## Validation

- `Test-AzsunaRescue.ps1`: compiles the complete production source against a test harness; teleport acknowledgement, duplicate suppression, actual boarding, ordered travel/floor checks, distinct enemy defeats, wrong-player/spell rejection, scene cancellation and retry, native meteor dispatch, reward/abandonment/death cleanup.
- `Test-AzsunaRescueNative.py`: eight objectives, native spells/seat/level scaling, scene package/timeline/location.
- `Test-AzsunaRescueData.ps1`: applies the actual migration twice to temporary copies; checks bindings, preserved unrelated/custom data, removal of shortcuts and duplicate conversation.
- `Test-WarlockRegression.ps1`: existing spell and scripted mover regressions pass.
- Release scripts compile. Final worldserver build and runtime startup are checked during deployment.

The level-100 boost, custom ship scenario, runtime configuration, certificates and extracted data are outside the change.
