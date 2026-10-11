# Stonedark Crystal (39425)

## Finding and change

Objective 279391 requests one Stonedark Focus (128343). TDB735 build 25549
records `Flags2=1`; the live objective had `Flags2=0`. Migration 10 restores
that value, scoped by objective, quest, type and item. `QuestDef.cpp` loads
this field and `QuestPackets.cpp` sends it to the client. Its client-side
meaning has not been established here; this is a confirmed data mismatch,
not evidence that it prevented completion.

No completion blocker was found in the item collection path. Drop chances,
item limits and NPC spawns were preserved.

## Evidence and reachable loot

- The archived TDB735 objective, Navarrogg ender and reward link 39426 are
  captured with hashes in `highmountain-stonedark-crystal-native-2026-10-11.json`.
- The production WDC1 reader loads native ItemSparse 128343 with MaxCount 0,
  Stackable 1 and LimitCategory 0. No item-limit hotfix is needed.
- [The contemporary quest recording](https://warcraft.blizzplanet.com/blog/comments/stonedark-crystal-highmountain-quests)
  shows obtaining one focus during Feltotem combat. It supports the objective
  and source family, not exact drop percentages. The TDB archive contains no
  creature loot rows for this item, so the current percentages were not
  replaced with guesses.

| Loot entry | Current chance (%) | Static spawns in area 7725 |
| --- | ---: | ---: |
| 95290 Feltotem Warmonger | 8.3608 | 23 |
| 96307 Feltotem Warmonger | 0.361 | 0 (17 elsewhere) |
| 96423 Feltotem Bloodsinger | 24.8209 | 1 |
| 96615 Feltotem Demonkindre | 10.9061 | 0 |
| 104323 Feltotem Bonehound | 8.5929 | 5 |
| 104328 Feltotem Bloodbinder | 25.5046 | 12 |

All six loot rows have QuestRequired 1, one item, no group/reference/currency,
and match their creature template's loot ID. Four source types have hostile,
unphased spawns in the local quest area. An unused source with no static spawn
does not block collection from the other sources.

`LootTemplate::AddEntry` places these positive-chance, group-zero rows in
Entries. `LootTemplate::Process` accepts LootMode 0 (no difficulty restriction).
`Loot::AllowedForPlayer` gates quest items through `Player::HasQuestForItem`.
This ordinary quest is unavailable for item credit in a raid group.

Navarrogg 95421 is both starter and ender, with refuge spawn 340571 on map
1220. Quest FlagsEx is zero, so reward processing consumes one focus. The
existing parallel group after 39381 still requires rewards for 39391, 39425
and 39588 before 39426; no new prerequisite changes were needed.

## Validation

- `Test-HighmountainStonedarkCrystalNative.py`: archive comparison and hashes.
- `Test-BloodtotemItemNative.ps1`: actual production ItemSparse reader for
  both Hag Feather and Stonedark Focus.
- `Test-HighmountainStonedarkCrystalData.ps1`: migration twice on temporary
  copies, unrelated objective preservation, objective/loot/spawns/ender/gate.
  `-Live` checks the applied data without writes.
- `Test-HighmountainBloodtotem.ps1`: compiles production HasQuestForItem,
  ItemAddedQuestCheck, ItemRemovedQuestCheck, CanCompleteQuest, inventory
  limits, prerequisites and the reward item-removal branch. Tests absent,
  wrong-item, raid, complete and rewarded states; collect, discard, recollect;
  duplicate notification; one-item reward removal with bank untouched;
  all eight reward subsets of both parallel groups. The fixture supplies
  inventory storage and scheduling; it is not a live client playthrough.

Client looting, presentation, any incidental Torok dialogue and the full
playthrough remain unverified. No boost, ship, scenario, pet or camera code
was changed. Next small batch: 39588, They Will Pay With Blood.
