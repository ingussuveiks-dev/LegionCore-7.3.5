# Bloodtotem opening (38912, 39372, 39373, 39873)

## Findings and changes

The live opening incorrectly serialized Witch of the Wood, Hags of a Feather and
I Have a Bad Feeling About This. Witch also had a late-story prerequisite and
an alternate incoming link from bonus objective 39371. All three now follow
38912 independently. Their negative exclusive group requires all three rewards
before 39374, An Audience with Torok. The stray 38912 -> 38913 addon link now
points to 39372, matching the native reward link. Other Highmountain branches
remain available after 38909.

TDB735 build 25549 requires **50** Hag Feathers (objective 279309, Flags2=1),
whereas live SQL required 30. The actual runtime WDC1 ItemSparse record 128380
also has MaxCount=30 and Stackable=30. Restoring the objective alone would make
it impossible to collect enough feathers. A targeted, client-notified runtime
hotfix raises MaxCount to 50; Stackable remains 30. Native data files and the
generic inventory rules are unchanged.

A quest-specific login handler reconciles carried feathers with the objective.
An old complete save with only 30 reopens; an incomplete save carrying 50
completes. Banked items cannot satisfy turn-in until withdrawn. Absent and
rewarded quests remain untouched. No items or quest rewards are granted by this
repair. Quest 39373 has FlagsEx=0; the existing reward handler consumes 50 items.

## Executability evidence

- Oakin 95256 has an unphased spawn at 5221.07, 4902.25, 711.837 and the expected
  start/end relations. 38912 itself has no native objectives.
- Witch 95311 has a hostile, unphased spawn at 5649.85, 4774.81, 676.646. Its
  entry directly matches the native one-kill objective; no credit alias needed.
- Hag 95310 has 23 local spawns and a quest-only 128380 loot row, 3–7 feathers.
  LootMode=0 is a valid wildcard in this core's `LootTemplate::ProcessWorld`.
- Cup 244625 is a spawned GOOBER. The generic use handler calls `KillCreditGO`
  for its entry, matching native objective 280076. Its zero quest-ID field does
  not suppress credit. Navarrogg 97667 starts 39873; spawned questgiver cage
  243402 ends it. No additional credit script is necessary.
- No quest disables or extra accept conditions were found for these tasks.

## Sources and verification

`highmountain-bloodtotem-native-2026-10-11.json` records the original TDB735
objectives, reward links, relations and archive hash. Reproduce with:

```powershell
python tools/tests/Test-HighmountainBloodtotemNative.py
tools/tests/Test-BloodtotemItemNative.ps1
tools/tests/Test-HighmountainBloodtotem.ps1
tools/tests/Test-HighmountainBloodtotemData.ps1
tools/tests/Test-HighmountainBloodtotemData.ps1 -Live
```

The ItemSparse probe uses the production DB2 reader and Release libraries.
The regression harness compiles extracted production prerequisite, inventory,
completion, reward-item and new login/hotfix handlers. It checks all eight
reward subsets, the original inventory failure, boundary counts, bank exclusion,
repeat login, old completed saves, unaffected absent/rewarded quests and exact
turn-in consumption. SQL testing applies the migration twice to temporary
tables and verifies objectives, endpoints, incoming links and unrelated data.

Contemporaneous route evidence agrees that the three tasks open together:
[WoW-Pro Highmountain route](https://www.wow-pro.com/highmountain-neutral/).
[Hags of a Feather (December 2015)](https://warcraft.blizzplanet.com/hags-of-a-feather-highmountain-quests/)
also lists 50, but the version-specific quantity is grounded in TDB735.
The [documented story progression](https://warcraft.wiki.gg/wiki/Rite_of_Blood)
places all three tasks before the audience.

Client validation remains necessary: accept all three after 38912, kill the
Witch, loot 50 feathers across two stacks, use the Cup, turn in at the cage and
Oakin, and confirm the audience appears only after all three rewards. Include
an old 30/30 save and a bank/withdraw/relog check. Item cache display, combat,
Cup visual and phasing have not been verified in game.

The actual 39374 audience encounter is the next separate audit. Its current
Oakin proximity/timed-list credit is not verified by this prerequisite repair.
Boost, ship tutorial, pets and scenario criteria code were not changed.
