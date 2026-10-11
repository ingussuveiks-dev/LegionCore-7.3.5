# Highmountain: Get to High Ground (38909)

## Evidence

The archived TDB735 build-25549 quest records and installed 7.3.5.26972 DB2 files confirm two objectives, in this order: lead Jale to Thunder Totem (94547, slot 0, amount 1, flags 1), then follow Jale to Mayla (95078, slot 1, amount 1, flags 3). Existing objective records already match; no slot migration is needed. The native quest reward link is 38912. Records and hashes are in `highmountain-highground-native-2026-10-11.json`.

Native summon 188583 creates Jale 95078 using SummonProperties 3679 (Control=1, faction 1665, Title=0, Slot=0, flags include 512). This core creates an uncontrolled guardian; it does not assign the controllable-guardian primary pet field. The original accept hook is retained, with a duplicate/eligibility guard.

[Wowhead's quest page](https://www.wowhead.com/quest=38909/get-to-high-ground) corroborates the two objectives and reports recovery after a hearthstone or flight. The [quest narrative and progression](https://warcraft.wiki.gg/wiki/Get_to_High_Ground) identify three branches available after returning: The Bloodtotem Tribe (38912), The Skyhorn Tribe (38913), and A Walk With the Spirits (40515). The TDB records identify those exact quests; runtime QuestLineXQuest identifies 38912 and 38913 as branch starters. The Backdoor (39579) is a later step, not a prerequisite for starting all three branches.

## Defects and repair

- The bridge marker 94547 repeatedly credited any nearby player. Mayla 93826 independently awarded the second objective by proximity, even without Jale or the first objective. Event object 1501 signalled **all** nearby Jale summons, whose SmartAI credited their owners before moving and despawned after 3.5 seconds. These competing credit paths are removed; Mayla's unrelated quest 49773 conversation remains intact.
- Personal Jale now follows her summoner. Lead credit requires the living companion and owner together at the existing bridge marker, or together at Mayla after a legitimate travel/reconnect recovery. The latter supports native hearthstone/flight returns rather than forcing a walk back across the bridge.
- After the first objective, Jale leads along three ground destinations: the existing event-object position, the existing SmartAI destination, and the existing Mayla spawn. Each movement leg requires a matching arrival callback. She pauses if the owner falls behind; final credit requires both Jale's arrival and the owner within 15 yards of visible Mayla. No other player's summon or movement can supply credit.
- Objective credit uses the existing quest-status API to refresh dependent phases and area spells. It does not generate synthetic creature-kill achievements.
- Failed movement times out without credit. Death, loss of phase, leaving the region, vehicles, flight, logout, map change and abandonment remove the companion. She recovers on eligible ground updates; a native quest-lifetime token rejects a previous acceptance's summon after same-tick abandon/reaccept. Recovery after the first objective resumes the closest remaining route point, without awarding the second objective merely for spawning.
- Login removes a saved second-objective credit when the first is missing. Properly complete and already rewarded saves remain unchanged; the repair does not claim to distinguish historically valid completion from an old proximity completion when both flags are already present.
- Restore `RewardNextQuest=38912` for 38909. Change the three branch `PrevQuestID` values from 39579 to 38909. The core merges incoming `NextQuestID` links as **alternative** prerequisites: the previous configuration still allowed Bloodtotem via 38909 and subsequent branches serially via 38912/38913. It did not correctly express all three branches opening together. Existing later branch contents and other links are retained for separate audits.

## Validation

- `tools/tests/Test-HighmountainHighground.ps1` compiles the extracted production summon gate, escort AI, player recovery handlers, core delayed-quest helper and quest status type. Tests cover actual arrival callbacks, two owners, wrong phase/visibility, distance pause/resume, repeated summons, failed movement/spawn, hearthstone-style arrival, reconnect, death, map/region/flight cleanup, same-tick abandonment and legacy save reconciliation.
- `tools/tests/Test-HighmountainHighgroundNative.py` verifies native objectives/slots/flags, the summon chain/properties, native follow-up link and evidence hashes.
- `tools/tests/Test-HighmountainHighgroundData.ps1` applies the migration twice to temporary copies. Validates bindings, removed competing credit, retained accept hook and unrelated Mayla conversation, physical endpoint spawns, branch prerequisites/reward link and unaffected data. `-Live` is read-only deployment validation.
- A read-only probe linked with the actual Release MMAP/Detour libraries found connected paths for bridge → event point → old Jale destination → Mayla (4, 31 and 26 polygons). Map 1220, tiles 24/22 and 24/23. The route is reconstructed from existing server coordinates, not claimed to be a retail movement sniff.
- Release script compilation succeeded. The final executable is built and startup-checked in `build-extractors/bin/Release`; runtime configuration and data are preserved.

## Client checks still required

Play through both objectives and the Mayla turn-in in the 7.3.5 client; check Jale's movement, ground/mounted travel, recovery after flying/landing, dialogue, visibility, pet portrait, two simultaneous players, and the three quest offers after turn-in. Ground navigation and script tests do not certify client animation or the complete retail narrative/ambush sequence. No substitute creature or spell IDs, boost changes, ship-scenario changes, pet-core changes or global criteria changes are included.

Next separate batch: **38912 — The Bloodtotem Tribe**, including Oakin's actual turn-in and the opening Bloodtotem objectives. Opening that branch here does not claim its later objectives have been audited.
