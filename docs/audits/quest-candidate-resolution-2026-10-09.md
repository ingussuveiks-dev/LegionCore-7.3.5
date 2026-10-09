# Follow-up: correct identities and links for the eleven quest candidates

## Confirmed correction

**34314 Out of the Chains -> 34315 Gearing Up -> 34316 Seeking the Truth.**

The runtime's native **7.3.5.26972 QuestLineXQuest.db2** contains line 115,
records 1615/1616/1617, with those quest IDs in order 1/2/3. The
[quest's documented series](https://www.wowhead.com/quest=34315/gearing-up)
independently confirms both neighbors. This is Shadow Hunter Bwu'ja's
**Draenor Garrison Campaign**, not a boost training quest. The earlier chat's
boost classification was incorrect.

Migration `2026_10_09_01_fix_gearing_up_chain.sql` atomically replaces the three
self-references on 34315:

| Field | Before | After |
|---|---:|---:|
| quest_template_addon.PrevQuestID | 34315 | 34314 |
| quest_template_addon.NextQuestID | 34315 | 34316 |
| quest_template.RewardNextQuest | 34315 | 34316 |

Incoming NextQuestID supplies 34316's prerequisite through the production quest
loader. No additional predecessor was fabricated. The SQL only runs when all
three fields still have the observed bad values.

Questgiver 78659 is spawned. Objective items 110606/110607/110608 have
quest-required 100% loot entries 52641/52642/52643, and matching chests
229415/229416/229421 are spawned. However, successor questgiver **78746 has no
ordinary or garrison-building spawn**, no AI/script binding, and no explicit
SmartAI summon found. The chain's data links are corrected; an end-to-end
playthrough and the successor's appearance are not verified.

## Correct IDs for the other candidates

| Candidate | Correct relationship/identity | What still prevents calling it fixed |
|---|---|---|
| 34027 | Previous quest **34026** | Root 34026 is disabled locally. |
| 34028 | Previous quest **34027** | Inherits that disabled root. |
| 34029 | Previous quest **34028** | Inherits that disabled root. |
| 34030 | Previous quest **34029** | Inherits that disabled root. |
| 36164 | Previous quest **36163** | Root 36163 is disabled locally. |
| 36167 | Previous quest **36163** | Same root. |
| 36168 | Rewarded-quest condition **36163** | Same root; these three trials are parallel, not consecutive. |
| 38923 | Horde **Ashran Dominance**, level 110 | Self-prerequisite and missing acquisition/event-credit implementation remain unresolved. |
| 39090 | **Slay Them All!**, level 110 | Self-prerequisite and missing acquisition/reset behavior remain unresolved. |
| 44556 | Original predecessor **44944 Aura of Uncertainty** | Disabled/obsolete attunement branch; no proven replacement established. |

### Seismic Matters: the existing IDs are real

Native QuestLineXQuest line **70**, rows **863–867**, contains
34026/34027/34028/34029/34030 in order 0–4. The
[documented Seismic Matters series](https://www.wowhead.com/quest=34026/garrison-campaign-seismic-matters)
agrees. These are not unidentified obsolete IDs needing renumbering.

Local prerequisites already match. Root 34026 has a sourceType=1 disable with
an empty comment. Quest participants 77217, 77225, 77160, 77161 and 77311 have
no ordinary or garrison-building placement, no AI/script binding and no
explicit SmartAI summons or spellclick entries found in this pass. Their
templates exist, which explains why a reference-only audit did not catch this.
Removing the root's disable would not provide the missing interactions.
No disable, coordinate or quest credit was guessed or changed.

### Exarch's Call: keep the parallel trials

Native line **106**, rows **1414–1417**, contains 36163 and the three trials.
The [documented series](https://www.wowhead.com/quest=36163/garrison-campaign-the-exarchs-call)
shows all three trials following the same introduction. DB2 display order
must not be converted into a linear 36164 -> 36167 -> 36168 prerequisite.

The existing links/condition are correct. The blocker is disabled root 36163.
Its starter Deedree 82776 and Yrel 73395 have placements, but the inspected
trial participants 84364, 84368, 84538, 84719 and 84814 have no ordinary or
garrison-building placements, explicit SmartAI summons or spellclick entries.
Their templates have no AI/script bindings. A generic spell/scene may still
contain part of a summoning path; absence from these checks is not proof of
every possible runtime path. The trial sequences require implementation tracing
and testing, not a substitute predecessor ID. Root 36163 remains disabled.

### Ashran: similarly named quests are not verified replacements

38923 is Horde-specific at 110. The suggested lookalike 38925 is Alliance at
100; 39096 is also Alliance/100, and 39524 is a separate level-100 repeatable
variant. Replacing the level-110 IDs with those records would alter faction,
level and repeatability semantics.

The [38923 page](https://www.wowhead.com/quest=38923/ashran-dominance) even has a
player report explicitly mentioning 7.3.5; it is evidence against assuming the
quest was already absent from Legion solely because today's page says obsolete.
The [39090 page](https://www.wowhead.com/quest=39090/slay-them-all) describes the
200-player objective. These sources do not establish the exact acquisition and
reset implementation needed by this core.

Both local records are QuestType=2, AreaGroupID=3856. Neither has an
area_queststart relationship, ordinary NPC starter or native QuestV2CliTask
record. `LoadQuests` only registers type-3 tasks for area auto-grants, so area
group alone does not make these normal quests start. The Ashran scripts contain
no explicit use of these quest IDs or event credit 95099. Neither record was
renumbered, enabled or silently classified as resolved.

### Karazhan: original link verified, replacement not verified

Native line **294** rows 4070/4071 retains **44944 -> 44556**. The existing
link is not a typo. Both current Wowhead pages mark the quests obsolete:
[44944](https://www.wowhead.com/quest=44944/aura-of-uncertainty),
[44556](https://www.wowhead.com/quest=44556/return-to-karazhan).
Retired quest lines can remain in DB2, so native presence is not evidence to
re-enable an old quest. No reliable version-specific replacement was found.

The later quests **44557 Finite Numbers**, **44683 Holding the Lines** and
**44684 Corruption Runs Deep** already have local starters and zero addon
prerequisites; map 1651 has no quest_done_A/H access gate. These are the existing
independent continuation points, not replacements to insert as 44556's previous
quest. No Karazhan link or access requirement was modified.

## Verification and remaining work

- SQL temporary-table test passes: all three fields corrected, second run
  changes nothing, other quest links untouched, and an altered prerequisite
  prevents a partial overwrite.
- Four graph regression tests pass, including the new 34314/34315/34316 case.
- Native DB2 hashes, storyline rows and the remaining candidates are saved in
  `quest-candidate-resolution-2026-10-09.json`. Runtime SQL overlay counts for
  quest_line_x_quest and quest_v2_cli_task were both zero.
- Migration applied to the runtime database. Graph candidates fall from ten
  to nine; the additional 36168 condition remains. This is **one repaired
  candidate, not eleven completed quests**.
- With zero online characters, worldserver was gracefully restarted from
  `build-extractors/bin/Release` to load the SQL change. No C++ rebuild required.
- Custom boost/ship scripts, scenario data and runtime configuration unchanged.

The correct identities and original links are now recorded. Full restoration
of disabled Garrison events, Ashran acquisition/credit/reset behavior and the
Gearing Up successor's appearance remain separate unfinished implementation
work. No in-client completion is claimed.

## Gameplay repair follow-up: Bloodmaul, 34309 through 34316

The subsequent implementation adds `draenor_campaign_recovery.cpp` and migration
`2026_10_09_02_bloodmaul_campaign_progress.sql`. This is a functional recovery of
the first four quests, not a claim that the entire campaign or all level 100–110
quests have been restored.

- **34309:** discovery credit 78060 requires the real prisoner 78659 within eight
  yards and line of sight. Accepting the quest does not award discovery credit.
- **34314:** objective 272536 is **sequenced (flag 2), not optional (flag 4)**.
  The missing shackle 229414 now appears personally at the existing prisoner's
  position while the player has key 110664. Its use handler rechecks ownership,
  quest status and the key, then allows the ordinary GO objective credit and
  native spell 159041. Abandoning, finishing the objective or losing the key
  removes it. The existing Rugrum loot row is retained: LootMode=0 is accepted
  by this core's `LootTemplate::ProcessWorld`; it is not a broken loot mode.
- **34315:** all three existing gear containers and their ordinary quest loot
  remain required. After hand-in, personal questgiver 78746 becomes available
  near the prisoner and is recovered after login/return.
- **34316:** reaching the actual quest POI polygon on the ground awards discovery
  credit 78252. Being dead, on a taxi, outside the polygon or above the ground
  does not. Personal receiver 78785 appears at the hand-in POI (7384, 5027),
  with terrain-resolved height. No objective is credited just for spawning him.
  He remains available for the existing next quest after hand-in.
- Prerequisites require rewarding 34309 before 34314, 34315 before 34316 and
  34316 before 34381. Quest objectives and rewards are unchanged. Actors are
  personal, expire outside the area and are explicitly cleaned on logout.

The [shackle interaction](https://www.wowhead.com/quest=34314/out-of-the-chains)
and [cave-arrival objective](https://www.wowhead.com/quest=34316/seeking-the-truth)
agree with the native/local objective records. Actor placement uses the existing
prisoner and native quest POIs; this recovery does not reconstruct companion
dialogue, escort animations or every original visual.

Validation: `Test-BloodmaulCampaign.ps1` compiles the production player/GO scripts
against a small world boundary and exercises ownership, key loss, abandonment,
login/map return, cleanup, quest order and discovery restrictions.
`Test-BloodmaulCampaignData.ps1` runs the production SQL in temporary tables and
checks idempotence, isolation, custom binding preservation, mandatory shackle,
key loot, gear goals and quest handoffs. Existing Draenor and Archaeology chain
SQL tests also pass. The production scripts compile in Release. Client playthrough
and visual confirmation remain outstanding; these tests are not a substitute.

### Remaining implementation blockers

- **34381 onward:** Grubnor 78003, gate 229026 and receiver 78792 are absent.
  The spirit-world transition and the following Orlana/soulgrinder sequence
  require implementation. The newly restored 34316 hand-in does not fix these.
- **34026–34030 and the Exarch trials:** entire event actors/interactions remain
  missing; the disabled roots have not been enabled over incomplete gameplay.
  Native summon spells 159067/159122 create scout actors 78753/78786, not the
  missing questgiver actors, so they are not a substitute for this repair.
- **Ashran:** the live `outdoorpvp_template` has no Ashran controller registration.
  Its source registers zone 6941, which native AreaTable maps to Draenor 1116;
  the battle map 1191 uses zone 8485. Native area 7279 is the racing stadium.
  The racing victory handler has no objective credit 95099. These are additional
  blockers beyond the previously identified acquisition/reset problem.
  Conversely, faction bosses 82876/82877 already have KillCredit1=98332, which
  matches CriteriaTree 46081 -> 46082 -> Criteria 29154 (kill 98332). Ordinary
  PvP kills already call `KilledPlayerCredit`; adding another copy would double
  count. No partial Ashran activation or duplicate credit was introduced.
- **44556:** the original 44944 predecessor is confirmed; a version-correct
  replacement remains unproven. No replacement ID was invented.

Custom level-100 boost, ship, bird, scenario criteria and pet paths are unchanged.
