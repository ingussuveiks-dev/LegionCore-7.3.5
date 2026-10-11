# Blood Debt (39426) and handoff to Step into the Dark

## Confirmed problems and changes

- Torok 95707 used SmartAI with only a death-notification row. SmartAI does
  not schedule `creature_template_spell` automatically. Four combat events
  now cast native Fel Slash 197427, Demonic Cleave 197450, Raging Blood 197500
  and Fel Rush 197520. Victim/self targets match their native effects. Existing
  death notification and ordinary kill credit are retained. The two other
  listed spells (213738/215377) were not indiscriminately enabled.
- Spell 191109 is a summon, not an aura. Both quest acceptance and area
  updates could cast it, while the area code only checked `HasAura`. The
  personal Navarrogg wrapper now rejects duplicate living summons. Dead or
  pending-despawn companions do not prevent replacement. It preserves the
  original SmartAI personal visibility, follow, Crystal Bolt and handoff.
- Missing Navarrogg is recovered with the native summon on login and at
  five-second intervals in areas 7725/7810/7829, while Blood Debt is active,
  complete or rewarded and 40229 has not been accepted. This keeps both the
  quest ender and next starter available after interruption. Recovery is
  blocked while dead, teleporting, flying or in a vehicle. Abandonment,
  logout, map changes and accepting 40229 clean up owned companions.
- Navarrogg had a completion talk row for missing text group 2, alongside
  the valid group-1 line. The missing-group row is removed.
- 40229's addon NextQuestID incorrectly pointed to 39318, Nursing the Wounds.
  TDB735 and the live RewardNextQuest both point to 39456, Unexpected Allies;
  that forward link is corrected. The unrelated 39318 prerequisite is not
  reconstructed without separate evidence.
- Legacy 40243 also supplies an incoming prerequisite to 40229. A quest-accept
  condition now requires 39426 rewarded, so that alternate incoming link
  cannot bypass Torok's defeat and turn-in. The legacy quest itself is not
  removed or made mandatory.

## Native evidence and scope

The adjacent native JSON records TDB735 archive hash, the actual 7.3.5 DB2
spell hashes/effects/names and summon properties. Objective 279393 is already
correct: kill 95707 once, storage index 1, Flags 1 and Flags2 0. Flag 1 is
minimap tracking, not a completion bypass. Step into the Dark has no numeric
objective and is handed in at Navarrogg 95392 in Stonedark Grotto, spawn
340565. Torok's existing spawn 283112 is on map 1220 at Torok's Bluff.

191109 summons 96453 with properties 3839 (Control 1, Slot 0, Flags 8522506).
The native summon path is retained; primary pet slots are not changed by
this script. Native Fel Rush triggers existing 197515. Demonic Cleave has
the native knockback effect; no replacement IDs or client visuals are invented.

[Blood Debt's quest record](https://www.wowhead.com/quest=39426/blood-debt)
corroborates Torok's defeat and the accompanying Navarrogg. Current retail
reward/level changes were not applied to the 7.3.5 server.

Combat event timing is server tuning, not a recovered retail sniff: initial
delays 2/8/15/20 seconds; repeats 8/15/45/20 seconds. Native spell casting and
effects remain authoritative. Client combat pacing, knockback/visuals,
companion presentation, pet portrait and the full playthrough remain unverified.
Boost, ship, scenario criteria, generic pet/mover and warlock production
paths were not modified.

## Validation

- `Test-HighmountainBloodDebtNative.py`: archive/DB2 records and hashes,
  objective, native follow-up link, four ability names/effects and summon.
- `Test-HighmountainBloodDebtData.ps1`: migration applied twice to temporary
  tables, all four cast events, retained death/personal/follow/handoff rows,
  valid dialogue, existing actors/enders, follow-up link and required reward
  condition; unrelated AI, SmartAI and quest links preserved. `-Live` is read-only.
- `Test-HighmountainBloodDebt.ps1`: compiles the production companion code
  with world-service fixtures. Covers all allowed quest states, next-quest
  exclusion, map/area/death/transport guards, duplicate suppression, pending
  removal, SmartAI delegation, owner-only cleanup and retry throttling.
- `Test-HighmountainBloodtotem.ps1`: actual production kill/completion
  handlers verify incomplete before Torok dies, exactly one completion,
  no double count from his Feltotem alias, plus prior item and chain tests.
  Objective flag constants in this fixture now match QuestDef.h.
- Release scripts compile successfully; final deployment uses the canonical
  `build-extractors/bin/Release` runtime and the ordinary SQL updater.

These checks do not simulate live combat or certify the client. Next audit:
the remainder of Step into the Dark / Unexpected Allies and the return to
Thunder Totem.
