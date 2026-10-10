# Eye of Azshara quest chain — 2026-10-10

Scope: 38286 (Wrath of Azshara) -> 42213 (The Tidestone of Golganneth).

## Proven defects repaired

- The entry credit 106847 was periodically awarded by a shared SmartAI template, including its spawn outside the dungeon on map 1220. Credit now comes from instance 1456's OnPlayerEnter, only with the active quest and without replacing either boss kill.
- The final event checked completion only once after initialization. Bosses/nagas/bubble loading in later grids could miss the check and remain locked. Relevant creation hooks now schedule the check after AI initialization. Four fixed naga slots replace the unbounded append index, avoiding overflow on repeat creation. The awakening announcement is emitted once.
- Tidestone NPC 106780 is available after Wrath is defeated, including when the NPC loads after a saved DONE state. It is hidden before that state. The existing turn-in and follow-up quest relations are preserved.
- Dalaran's decorative upper/lower teleport pads had no server action. Their existing spawns now cast native 192293 / 192295 when an alive player walks onto the pad. Existing destinations are used; arrival locations lie outside the activation radius. Vehicles, flights, other maps/floors and pending teleports are excluded.
- The 42213 teleport objective was awarded merely near credit bunnies. That specific proximity action is removed. Only the upper pad's successful, acknowledged arrival downstairs awards 106815; failed/pending teleports do not. The separate 109750 action for quest 40890 remains unchanged.
- 38286 incorrectly used the belt package from 42213. TDB 7.3.5 specifies one Tidestone Sliver (141385), with no package. Restored those fields. The following quest retains native package 18462, containing four armor-type belt choices.

## Verified existing links

All four objective IDs, types, amounts and storage slots match TDB 7.3.5. The previous quest 37470, follow-up 42213, Farondis questgiver, Tidestone NPC questender/starter and Dalaran gameobject 246465 questender are present. All five bosses have their registered scripts and spawns supporting normal/heroic/mythic difficulties. The final boss becomes attackable after its four naga die; the kill objectives use ordinary core kill credit.

42213 supplies item 137206 (ProvidedItemCount=1) and lists it in ItemDrop1 with quantity 1. The existing RewardQuest path removes ItemDrop items and calls TakeQuestSourceItem; no extra deletion script is needed. The Dalaran questender's phase 4586 is enabled until 42213 is rewarded. Existing objective storage and player progress are not rewritten.

## Evidence and validation

- Native build 26972 SpellEffect, Item, QuestPackageItem and GameObjectDisplayInfo DB2, plus TDB 7.3.5 quest/objective archive. IDs and hashes: `eye-quest-chain-native-2026-10-10.json`.
- Existing live pad spawns and spell_target_position supply the two-way route; no destination coordinates or replacement IDs were invented.
- [Wowhead 38286](https://www.wowhead.com/quest=38286/eye-of-azshara-wrath-of-azshara) corroborates dungeon entry, both kills and the Tidestone Sliver reward.
- [Wowhead 42213](https://www.wowhead.com/quest=42213/eye-of-azshara-the-tidestone-of-golganneth) corroborates the central teleporter and Portrait Room turn-in. Current level/reward changes are not used as 7.3.5 values.
- `Test-EyeQuestChain.ps1` compiles extracted production instance methods plus the complete portal source. Covers late grids, repeated naga creation, locked/unlocked states, entry credit, pending/failed/completed teleport, wrong map/floor/quest, death/vehicle, return teleport and bounce prevention.
- `Test-EyeQuestChainNative.py` checks the native records and writes the evidence report.
- `Test-EyeQuestChainData.ps1` applies the real SQL migration twice to temporary tables and validates unchanged objectives/progress fields, rewards, relations, phase, spawns, bindings and preservation of unrelated/custom scripts.
- Release scripts compile; final Release build/startup are checked during deployment.

No client playthrough was performed. This is a quest-progression audit, not certification of every boss ability: the existing disabled instance weather update and disabled Arcane Bomb boarding remain outside scope. Pad effects, all boss combat, group kill credit in practice, the actual turn-in UI and the placed Tidestone's appearance still require a 7.3.5 client test. Boost, ship scenario, shared pet/mover code and runtime configuration are untouched.
