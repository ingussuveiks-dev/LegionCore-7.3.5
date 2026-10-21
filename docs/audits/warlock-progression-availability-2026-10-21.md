# Warlock progression availability after artifacts, 2026-10-21

Scope: the recommendations given to level-106 Maiko: the remaining Warlock
101/103 campaign, champions, troop/research/mission interactions, four scouting
zone entries, and the level-110 Suramar/World Quest entry gates. This is an
availability and interaction audit, not certification of every quest objective
or the whole Legion expansion in the client.

The character was read only. No levels, quest states, followers, currency,
items or personal progression were granted or reset. Natural questgivers are
used; the custom class-quest assistance NPC 400005 is excluded as proof of
availability. Existing quest audit failures and client-pending status remain.

## Confirmed blocker and implementation

Player::PrepareGossipMenu read only UNIT_FIELD_NPC_FLAGS (32 bits), while
GetRequiredNpcFlagForOption returned secondary service bits as unshifted
values. Consequently an unrelated primary flag could admit a service, and
correct secondary flags could not admit it.

Two immediate Warlock blockers follow from the actual data:

- Imp Mother Dyala 106216: primary 1, secondary 272, menu 19918/option 28.
  Her troop order option failed the first filter before reaching the native
  canAddShipmentOrder quest/site/class checks.
- Archivist Melinda 108018: primary 1, secondary 512, menu 19812/option 32.
  Her research option failed the first filter. The core also incorrectly
  aliased CLASS_HALL_UPGRADE to RECRUITER at 256. All twelve natural class-hall
  research templates with option 32 use secondary 512 in the database.

The fix combines the two fields into a uint64 mask and shifts secondary
requirements into its high half. Troop order admission matches the existing
canAddShipmentOrder and shipment packet guards (AI_OBSTACLE 4 or RECRUITER
256). Recruitment remains 256 and research is corrected to 512. Architect,
mission and tradeskill options also use the correct secondary field. Scouting
option 31 continues to require primary gossip 1, including before champions
enable the board's secondary mission flag. No NPC flags or eligibility gates
are bypassed to make these menus appear.

## Natural campaign path at level 106

| Quest | Natural interaction and gate | Result of automated audit |
|---|---|---|
| 44099 A Mutual Friend | City messenger after rewarded 40824 | Previous scoped spell_area fix retained; city appearance still needs client confirmation. |
| 42608 Rise, Champions | Calydus 101097 in Dreadscar, base phase 6930; rewarded 44099 | Native level/prerequisite admission passes in a progressed fixture. |
| 41748 / 41750 Champion quests | Ritssyn 104795 and Calydus; require active 42608 | Native conditions admit during 42608 and reject after it is complete/rewarded. Phase 6933 exposes Ritssyn/Gakin. |
| 42603 Information at Any Cost | Gakin 106199; rewarded 42608 | Mission 827 exists, class-order/follower type matches, 120-second native duration. Completion criteria asset 827 matches native mission completion dispatch. |
| 41797 Recruiting The Troops | Gakin -> Imp Mother Dyala 106216; phase 6934 after 42603 | Menu filter repaired. Native order gate rejects missing site/quest/class; admits the active quest and opens container 129. |
| 42602 Troops in the Field | Gakin, after 41797 | Mission 928 exists; native duration one hour; completion criteria asset 928. |
| 42601 Tech It Up A Notch | Gakin -> Archivist Melinda 108018; phase 6935 after 42602 | Research menu repaired. Native canStartUpgrade requires the class's research quest. Successful research uses the existing post-cost AchieveCriteriaCredit(50169) path; no new credit shortcut. |
| 42097 Searching the Archives | Ritssyn 104795, after 42601 | Mission 839 exists, one-hour native duration, matching completion criterion. |
| 41759 An Unlikely Ally | Ritssyn -> Kira Iresoul 105816 in Val'sharah | Level 103 and rewarded 42097; natural Kira spawn and questgiver flags present. |
| 39179 -> 39389 -> 39142 -> 40218 | Kira's Bloodstone chain -> Ritssyn 105018 | All native level/race/class/prerequisite gates pass sequentially. Ritssyn phase 6936 admits active/complete/rewarded 40218, subject to the later campaign gate. Objective gameplay is client-pending. |
| 41767 A Daring Rescue | Ritssyn 105018 after 40218 | Admission and subsequent phase 6938 gates pass. Its actual rescue/object interactions are not claimed verified. |
| 41752 / 41753 / 41798 | Zinnin/Jubeka champions and additional troops after 41767 | Natural giver spawns/phases exist; native admission passes. Jared phase 6939 admits active/complete/rewarded 41798. |
| 42100 -> 42098 | Empowering the Soul -> Black Rook Hold: An Unclaimed Soul | Admission passes after prerequisites. Native normal Black Rook Hold LFG 1204 is minimum 110, even though the quest can be accepted at 103. Finish this dungeon step at 110. |

Native DB2 confirms champion reward spells 201163 -> follower 589 and 207247 ->
follower 616, matching the two Rise, Champions criteria. The troop fixture uses
actual records: shipment 255 has unfinished-quest flag 1; shipment 321 has flag
0 and is the ordinary post-quest order. Completion spell 210566 uses native
effect 90 for credit 106208. The initially drafted fixture had these shipment
flags reversed; it was corrected from the native catalog before handoff.

## Zone and level-110 recommendations

Maiko's saved history already rewards 39718 (Azsuna) and 39731 (Val'sharah).
Those are not fresh scouting offers. Native admission accepts their next
quests 41220 and 39861; natural starters are Archmage Khadgar 86563 and
Malfurion 91172, respectively. Highmountain 39733 and Horde Stormheim 39864
remain valid fresh entries in the progressed fixture. Alliance Stormheim
39735 is rejected for the fixture's Horde race. The existing scouting tests
cover all fifteen natural board templates, the map packet and exactly-once
zone-selection credit. Actual client POI display and travel remain separate.

Suramar begins at level 110: 39985 Khadgar's Discovery is an area-start quest
on area 7506 (Violet Citadel), not a missing ordinary creature starter.
Khadgar 90417 is the natural ender. Uniting the Isles 43341 is also level 110
and its Source-19 conditions require Friendly (rank mask 16) with all five
factions 1828, 1859, 1883, 1900 and 1948. Native condition evaluation rejects
106, rejects one Neutral reputation at 110, and admits all five Friendly at
110. Reaching 110 alone does not satisfy that local first-character gate.
There are no disabled quests among the audited entries.

## Automated checks and limits

- Test-AdventureMapGossip.ps1: actual flag assembly/admission, all fifteen
  scouting boards, packet/selection handlers, and extracted canAddShipmentOrder,
  canStartUpgrade, selectShipment and OnGossipSelect. Missing site, no quest,
  wrong class, active/rewarded quest, tutorial/normal orders and cross-field
  bit collisions are covered.
- Test-WarlockProgression.ps1: actual Player level/race/class/prerequisite
  functions and ConditionMgr's real condition cases, negation and ElseGroup
  dispatcher, supplied with live world definitions. Covers the sequential
  campaign, champion and actor phases, zone follow-ups, level 110 and reputation
  gates. These fixtures do not instantiate a live Player, render NPC models,
  or exercise every CanTakeQuest subsystem or every objective.
- WarlockProgressionCatalog.cpp: native packed DB2 loader for follower/mission
  criteria, spells, shipments and Black Rook Hold. It also calls the actual
  gossip lookup from the original final-link game.lib.
- verify_gossip_service_release.py: calls the pure lookup in the actual x64
  Release image, mapped without process initialization/import resolution.
  All seven service masks are checked, so a fresh standalone test binary or
  a string in an old executable is not the only evidence.
- Test-DreadscarPlans.ps1, Test-ScepterLogin.ps1 -VerifyReleaseBinary and the
  ten-call scenario-LFG Release check passed. All SQL fixtures are temporary;
  this fix changes production C++ only.

Client checks remain for messenger/portal traversal, the two champion turn-ins,
troop collection, the research window/purchase, mission start/return/rewards,
the later rescue and item objectives, map pins, travel and level-110 popups.
Do not describe the whole campaign as client-verified from these results.

## Deployment

Release libraries were built at the original generated static-library paths;
the validation link disabled project reference building before changing only
the executable output directory. The tested executable and matching MAP were
copied into build-extractors/bin/Release while user servers were stopped.
Runtime configuration, certificates, DLLs, logs and extracted data were
preserved. SHA-256:
`b9fa03fb41b3160cd663254c5632391284c9c01665108b48d270356208b42838`.

The deployed EXE passed the direct service-mask, Scepter and scenario-LFG binary
checks. An owned hidden validation worldserver (PID 28448) reached ready at
13:16:27 on 2026-10-21, received `server shutdown 0`, and exited successfully
after closing its databases at 13:16:31. No validation server remains running.
Its stderr and DBErrors.log were empty. The 620 Server.log lines belonging to
this run contained no ERROR entries. An initial aggregate check also counted
historical errors in the appended Server.log; those predate this run and were
retained, then excluded by the current-run timestamp check. Startup does not
replace the client checks listed above.
