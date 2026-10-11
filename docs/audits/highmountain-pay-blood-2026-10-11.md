# They Will Pay With Blood (39588)

No confirmed data mismatch or completion blocker was found. No production
code, SQL migration or runtime restart is needed for this audit.

## Native data and live world

TDB735 build 25549 has objective 279704: monster 95290, amount 10, storage
index 0, Flags 0, Flags2 0. All of these match the live database. The archive
hash and relevant records are captured in the adjacent native evidence JSON.

The following mappings also match TDB735 exactly:

| Entry | Creature | Credit for 39588 | Static spawns in local area 7725 |
| --- | --- | --- | ---: |
| 95290 | Feltotem Warmonger | Direct objective | 23 |
| 96423 | Feltotem Bloodsinger | KillCredit1 = 95290 | 1 |
| 104323 | Feltotem Bonehound | KillCredit1 = 95290 | 5 |
| 104328 | Feltotem Bloodbinder | KillCredit1 = 95290 | 12 |
| 95707 | Torok Bloodtotem | KillCredit1 = 95290 | 0 (one in area 7810) |
| 100055 | Feltotem Bloodsinger | KillCredit1 = 95290 | 0 (one in area 7807) |
| 96615 | Feltotem Demonkindre | KillCredit1 = 95290 | 0 |
| 105065 | Feltotem Bloodbinder | KillCredit1 = 95290 | 0 |
| 105066 | Feltotem Bonehound | KillCredit1 = 95290 | 0 |
| 96307 | Cave Feltotem Warmonger | None in both databases | 0 (17 in cave areas) |

The 41 local spawns are hostile, have no explicit phase ID and respawn after
180 seconds. There are enough ordinary targets without Torok or an unspawned
template. No new spawns or name-based credit aliases were invented. Native
Bonehound credit is retained even though the quest text describes tauren.

Navarrogg 95421 starts and ends this quest at refuge spawn 340571. The quest
is enabled. Its existing parallel group requires rewards for 39391, 39425
and 39588 before 39426. No acceptance condition adds an unexpected restriction.

[The quest listing](https://www.wowhead.com/quest=39588/they-will-pay-with-blood)
corroborates ten Feltotem kills and multiple source types; current retail
levels and rewards were not used to alter the 7.3.5 data.

## Production path and tests

`KillRewarder::_RewardKillCredit` dispatches a 50 ms callback to
`Player::KilledMonster` for the killed creature. That method credits the
direct entry with its GUID and each alternate KillCredit without that GUID,
so the alternate ID is not replaced by the original creature entry in
`KilledMonsterCredit`. No SmartAI action duplicates credit 95290.

QuestData derives the kill/cast special flags from the monster objective.
The production quest handlers reject ordinary raid-group credit, increment
one objective point per kill and stop at the requested amount.

Validation:

- `Test-HighmountainPayBloodNative.py`: native objective, ten template
  mappings, starter/ender and next quest, with archive hash verification.
- `Test-HighmountainPayBloodData.ps1`: read-only checks of those mappings,
  objective, 41 local spawns, quest availability and refuge gate.
- `Test-HighmountainBloodtotem.ps1`: compiles actual `KilledMonster` and
  `KilledMonsterCredit`, plus `CanCompleteQuest` and prerequisite handlers.
  Each of the nine eligible template paths is exercised through 12 kills:
  exactly one point per kill, incomplete at 9, complete at 10, capped at 10,
  and one completion call. Absent, rewarded and raid quests are ignored;
  cave Warmonger 96307 is excluded. Existing item and all-eight-subset
  prerequisite tests still pass.

The test fixture supplies world lookup and achievement services. It does not
simulate full combat, pet ownership, party-distance distribution or client
presentation. These and a full in-game playthrough remain unverified.
Boost, ship, scenario, pet and camera production code were not touched.

Next batch: 39426, Blood Debt (Torok encounter and subsequent handoff).
