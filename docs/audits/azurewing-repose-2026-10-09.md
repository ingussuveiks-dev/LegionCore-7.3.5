# Azurewing Repose: progression and interaction repair

Scope: the 19 quests from **Journey to the Repose (38443)** through
**Hunger's End (42756)**, including the optional Stellagosa breadcrumb and pylon
side quest. This is a server-side audit and repair, not an in-client completion
certificate. The previous Faronaar work ends at 37449, the existing prerequisite
of 38443.

## Evidence

The TDB 7.3.5 archive dated 2018-02-19 supplies 31 objectives verified at build
25549. Runtime DB2 build 26972 supplies the original spells, action buttons,
vehicle and item effects. Records and file hashes are retained in
`azurewing-native-2026-10-09.json`. All 31 live objective IDs, quest IDs, types,
targets, amounts and flags match the archive after the migration. Existing SQL
storage indices are deliberately retained, preserving already saved progress;
the TDB ordering/storage differences are not mistaken for missing targets.

The archive has no objectives for delivery quests 38443, 37855, 37858 and 42567.
Their empty objective lists are valid. The unrelated old 37854 is not added to
this chain. No substitute quest or target IDs are invented.

[Blizzard's support reply](https://eu.forums.blizzard.com/en/wow/t/runas-knows-the-way-stuck-quest-need-some-help/9017/5)
confirms the whelplands prerequisites, particularly The Consumed, before Runas
Knows the Way. The native questline and
[documented progression](https://wowpedia.fandom.com/wiki/Runas_Knows_the_Way)
also place the two whelplands tasks before it. The existing Runas quest prerequisite
is retained and the two missing reward conditions are supplied.

## Confirmed defects and repairs

- **Crystals:** GO 240033 and 240267 had native loot rows for 122188 and 122306,
  but `Data1`/`chestLoot` was zero. `Player::SendLoot` therefore never called
  `FillLoot`. Link each chest to its existing loot table; keep its spawns and
  drop data.
- **The Death of the Eldest:** action bar 573 exposes 179915; Senegos only credited
  a hit from unrelated dummy 6967. The real button now requires six cores, the
  active quest, proximity and line of sight to Senegos. It consumes those cores,
  uses native throw visual 179913 and grants the pool objective once. The native
  spell's obsolete periodic trigger 179917 is suppressed.
- **Their Dying Breaths / The Consumed:** native revival spells credited their
  caster without verifying the clicked whelp, crystal requirements or previous
  use. Credit now requires the correct live, nearby whelp and quest items. A
  per-character ledger keeps distinct uses across relog; progress rollback or
  abandoning/reaccepting clears superseded uses on the next interaction. Other
  players and quests are independent. Existing native departure animations remain.
  Agapanthus's personal whelp still supplies item 122292 through native 180515;
  its pickup is owner/quest/range gated and obsolete trigger 180731 is suppressed.
- **Runas availability:** phase rule 15 negated `QUEST_NONE`, so the initial
  Stellagosa spawn required the quest she gives to have been taken already.
  Correct the polarity; the separate active-quest phase remains intact.
- **Runas's defeat and escort:** replace the exact-50%-health SmartAI poll with
  damage-based surrender, including scaled lethal hits and player pets. The old
  area summon could appear before the duel and grant arrival credit after one
  second anywhere. A personal follower is now recovered only after defeat, and
  arrival requires the owner and follower at the original Senegos location with
  a real visible Senegos. Death, abandonment and leaving the map clear it.
- **Runas Knows the Way:** keep the original ten route positions. A personal
  guide waits between legs if its owner falls behind; only actual arrival with
  the owner nearby and in line of sight grants credit. Repeated gossip reuses a
  living guide; failed summons and abandoning the quest grant nothing.
- **Pylons:** restrict the real extra-action spell to the four native pylon
  entries. A real hit supplies the hidden first-siphon credit; existing SmartAI
  supplies each individual pylon objective. Suppress missing trigger 179858 at
  the launch hooks, where this core actually executes trigger effects.
- **Still Alive:** the archive flags its flight objective as optional/hidden
  (28), not mandatory (2). Restore that flag and allow the ride while the delivery
  quest is complete but un-rewarded. Remove the old C++ implementation that
  credited the summon immediately, put personal visibility on the player and
  requested a nonexistent `waypoint_data` route. The replacement uses the ten
  existing `waypoints` positions and the existing ground-disembark position.
  Movement waits for confirmed boarding; early exit, failed boarding and quest
  removal do not credit the ride. Ground arrival records only this optional
  objective and exits the vehicle normally. Walking back remains valid.
- **Hunger's End:** Orbyth 91155 and Ael'Yith 108721 incorrectly credited each
  other's objectives. Ael'Yith also had no spawn/summon path. Remove those cross
  credits and Orbyth's erroneous player-board spellclick. Recover a personal
  Orbyth at its original spawn, then Ael'Yith at that encounter position only
  after Orbyth's actual kill. Spawning is never a kill. A saved first kill survives
  relog or death; an abandoned attempt restarts from its saved objectives.

Five item objective Flags2 values and the zero amount on hidden objective 280835
are restored from the archive. These changes do not reset characters, invent
rewards, alter loot chances or make every quest a linear prerequisite.

## Remaining original paths traced

| Quests | Server path |
| --- | --- |
| 38443, 37855, 37858, 42567 | Existing delivery starter/ender and predecessor relations |
| 37991 | Existing Agapanthus proximity objective and questgiver |
| 37856, 37960, 37861, 38014 | Existing attackable spawns and core kill objectives |
| 37959 | Existing quest-required mana-jewel loot and source creatures |
| 38015 | Start item 138146, native effect 214482, four explicit target conditions and per-caster SmartAI healing credit |

This traces their availability and execution mechanisms. It does not prove every
phase presentation, animation or combat encounter by playing it in a client.
Broad zone phase-list cleanup and reconstruction of all retail conversations,
cinematics and enemy abilities are outside this repair. Runas and the two final
enemies currently use basic combat. The final Ael'Yith placement is a recovery at
Orbyth's existing encounter anchor, not a claim to have recovered his retail jump.

## Validation and handoff

- `Test-AzurewingNative.py`: read-only archive/DB2 evidence checks.
- `Test-AzurewingData.ps1`: applies the actual migration repeatedly to temporary
  table copies, compares all 31 native objectives, preserves saved storage and
  unrelated spawns/custom bindings, and exercises ledger uniqueness/reset.
- `Test-AzurewingRepose.ps1`: compiles the complete production handlers against
  world/database boundaries. Covers item/range/LOS checks, persistent distinct
  revivals, scaled lethal damage, actual escort movement, separate owners,
  failed summons, boarding, early exit, arrival and sequential final progression.
  Engine loot generation, client input and normal death-credit dispatch remain
  explicit boundaries, not simulated claims of an in-game test.
- Production Release compilation and live migration/startup are checked during
  deployment to `build-extractors/bin/Release`.

Client confirmation is still needed for the full playthrough, pool/whelp effects,
guide pathfinding, passenger camera, dismount and pet portrait. The custom
level-100 boost, ship/Broken Shore scenario, shared vehicle/mover code, pet lookup
and warlock handlers are unchanged.
