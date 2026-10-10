# Highmountain opening: Dalaran, Thunder Totem and Earthmother's Bounty

Scope: 39733, 38907, 38911, 39491, 39272, 39490 and 39496. This batch ends at
Angler Creel in Whitewater Wash. It does not claim the remaining Rivermane or
Highmountain chapters are implemented or verified.

## Evidence

- Runtime 7.3.5 DB2: QuestLineXQuest 144, SpellEffect, TaxiNodes 1719 and its nine
  incoming TaxiPath entries. Spell 198160 discovers the existing Thunder Totem
  taxi node; 195481 is the native dummy spell used by the idol spell-clicks.
- TDB 735.00, build 25549 quest definitions/objectives and captured quest POIs:
  7 quests, 11 objectives and four different idol objective IDs. The stored
  POI coordinates map the existing four world spawns to entries 99433..99436.
- Exact rows and file hashes: `highmountain-intro-native-2026-10-10.json`.
- [The Lone Mountain](https://www.wowhead.com/quest=39733/the-lone-mountain)
  and [Keepers of the Hammer](https://www.wowhead.com/quest=38907/keepers-of-the-hammer)
  corroborate the Dalaran/Thunder Totem destinations. Current-site rewards are
  not used to replace 7.3.5 data.

## Repairs

1. Remove four duplicate acceptance-time completion commands for 39733. Restore
   native hidden/optional objective flags 28 while retaining its event gate;
   the existing actual arrival at Oro completes the event. Add an alive guard.
2. Replace 38907's acceptance-time flight credit with the existing close-gossip
   action and keep the linked 198160 discovery spell. A quest-owned observer
   watches the actual final taxi leg to node 1719, then checks dismount and
   proximity to its native landing point before awarding credit. It does not
   start a scripted flight or teleport. Walking/teleporting to the destination
   alone grants no flight credit. This credit is scoped to 38907, since Broken
   Shore quest 45571 also uses objective entry 96813.
3. Oro 106244 now grants the meeting objective only to a living, visible player
   on the quest, within eight yards on the correct floor, outside taxi/vehicle
   travel. Existing welcome/elevator dialogue is sent per player; delayed
   dialogue is guarded by the quest attempt. Remove Ebonhorn 93805's incorrect
   turn-in relation; Mayla 93826 remains the native ender.
4. Correct three of the four existing idol spawn entries using native POI
   positions. The native 195481 spell must actually hit an idol. Record its
   hidden objective before incrementing the common counter; repeat clicks on
   the same idol cannot replace visiting all four. These objective counters
   persist with ordinary quest data and clear with normal abandon. Shared
   idols are not despawned or disabled for other players. Client presentation
   of the native spell still needs in-game checking.
5. Restore Ormgul's item objective Flags2=1. Remove duplicate package 679 from
   Poisoned Crops; the package remains on Ormgul the Pestilent.
6. Delete only the corrupt duplicate 38907 POI row with impossible map/object
   IDs. Keep the native (BlobIndex=2, Idx1=5) row and all valid points.

Ormgul's existing quest-item loot was investigated and retained. In this core's
non-grouped `LootTemplate::Process`, LootMode=0 imposes no difficulty restriction;
it is not a missing-loot defect. His spawn, loot binding, quest-required item
128397 and level scaling are present. Infestation's two enemy types and all
seven quests' required starter/ender relations are present after the repair.
The shared chain has no faction restriction. The arrival NPC's existing phase
set overlaps the outer Highmountain base phase, while Mayla's lower-hall spawn
is unphased; no phase IDs or replacements were invented.

## Recovery and limits

The flight observer uses `AddQuestDelayedEvent`, so an abandoned attempt cannot
credit its replacement. Login restarts observation, including a resumed taxi.
Death, wrong map, teleport-in-progress or landing elsewhere discard the pending
arrival observation. If a disconnect finishes a flight entirely before the
new login can observe it, another actual flight to Thunder Totem is needed;
proximity alone is deliberately not treated as proof of flight.

There were zero active character rows for these seven quests before deployment.
No character migration or progress reset is applied. Existing Aludane actions,
including Broken Shore, remain unchanged. Their pre-existing shared 96813 credit
is outside this batch; the new Highmountain handler itself does not advance 45571.
No boost/ship scenario, generic flight/mover, pet or shared criteria code changed.

## Validation

- `Test-HighmountainIntroNative.py`: native quest/objective/POI data, ordered
  first Rivermane chapter entries, click/discovery effects and taxi destination.
- `Test-HighmountainIntroData.ps1`: actual SQL migration twice in temporary
  table copies; all 11 objective layouts, four unique POI-matched idols, native
  click bindings, starter/ender links, rewards, original loot and custom/other
  scripts. `-Live` reads the installed state without applying migrations.
- `Test-HighmountainIntro.ps1`: compiles actual production handlers and the
  quest-owned scheduler. Covers actual taxi arrival, wrong-route/walking/
  teleport/death rejection, abandon/reaccept, login during flight, isolated
  quest credit, Oro phase/range guards and distinct idol hits for two players.
- Full Release compilation and startup verification use the canonical
  `build-extractors/bin/Release` runtime.

Still requires a 7.3.5 client run: Dalaran taxi routing/UI and flight cost,
Thunder Totem's elevator and both floors' visibility, dialogue and idol spell
visuals, Ormgul loot acquisition and the complete seven-quest playthrough.
Next bounded batch: Trapped Tauren / Spray and Prey / Fish Out of Water.
