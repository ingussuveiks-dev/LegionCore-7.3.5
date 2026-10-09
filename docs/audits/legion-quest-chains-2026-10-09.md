# Level 100–110 quest chain audit — 7.3.5.26972

Follow-up: [candidate resolution](quest-candidate-resolution-2026-10-09.md)
identifies 34315 as Shadow Hunter Bwu'ja's Garrison Campaign quest and repairs
34314 -> 34315 -> 34316 using native DB2 evidence. The counts and JSON below
are the initial audit snapshot, before that second repair. Remaining ordinary
graph candidates are now nine, plus the separate 36168 condition candidate.

## Result and scope

One confirmed availability defect was repaired: the rotating Archaeology chain
**41183 → 41184 → 41185** had a self-prerequisite on its first quest. This is a
repair of the chain's entry gate, not certification of all three quests in game.

The read-only scan includes 7,228 quest templates, 9,850 objectives, 9,641
NPC/gameobject/area relationships, 739 availability conditions and 22 event
relationships. Nine quests in this scope were already disabled. The selection
is `MinLevel 98..110 OR QuestLevel 100..110 OR Expansion=6`, so it deliberately
includes Legion introductions at 98, level-110 endgame/professions and some
Draenor/older content. Many Legion rows have Expansion=0; filtering only on 6
would miss them. Titles come from the enUS locale where available.

Checks found:

- No missing PrevQuestID, NextQuestID or RewardNextQuest target in the scope.
- No missing questgiver/ender creature or gameobject template in the scope.
- No missing creature/gameobject objective target in the scope.
- Thirteen quests initially lacked an entry through the optimistic predecessor
  graph; ten remain after the Archaeology repair.
- One additional positive availability condition depends on a disabled quest.
- 1,440 source relationships have no ordinary static spawn. These are an
  inventory for investigation, **not** 1,440 proven bugs: scripted summons,
  transports and conditional spawns require separate tracing.

The JSON stores the post-repair candidates and SHA-256 hashes of the input
exports. Exports contain no character/account data. The previous broader
`progression-lifecycle-2026-10-09` audit covers DB2 item/spell/currency references;
this pass does not rerun every DB2 table or claim every objective is executable.

## Repair: Academic Exploration

Before:

| Quest | PrevQuestID | NextQuestID |
|---|---:|---:|
| 41183 Academic Exploration | 0 | **41183** |
| 41184 Tried and True | 41183 | 41185 |
| 41185 The Keys to Success | 41184 | 0 |

`QuestData::LoadQuests` adds a quest's NextQuestID to the destination's previous
quest list. Consequently, 41183 required 41183 to have been rewarded before it
could be accepted. Its descendants inherited that inaccessible entry point.

The migration changes only 41183.NextQuestID from 41183 to **41184**, and only
when PrevQuestID is still zero. Both the local successor's PrevQuestID and the
[Wowhead storyline](https://www.wowhead.com/quest=41183/academic-exploration)
support this ordering. Archaeology 794/rank 700, event 165, questgivers 93538 and
103482, and all other prerequisites are preserved. No replacement ID was guessed.

Both questgivers have runtime spawns. Item 134117 for 41184 has quest-required
loot entries (creature loot 102777; gameobject loot 64396, 71840 and 268453).
Native SpellEffect data contains create-item effects for 41185's intermediate
items and final key: spells 205778, 205779, 207934 and 205781. Their existence
alone does **not** verify the blacksmith's gossip/extra-action sequence. That
sequence, archaeology drops and turn-ins still need an end-to-end gameplay test.
The [quest description](https://www.wowhead.com/quest=41185/the-keys-to-success)
provides the intended blacksmith workflow.

## Other candidates: no speculative changes

| IDs | Finding | Disposition |
|---|---|---|
| 34027–34030 | Draenor Garrison Campaign descendants of disabled 34026 | Blocked for a fresh character under ordinary prerequisite checks. No proven replacement or bypass; left unchanged. |
| 36164, 36167 | Draenor trials depend on disabled 36163 | Same limitation; left unchanged. |
| 36168 | `conditions` SourceType 19 requires rewarded 36163 | Additional condition-level blocker. Not found by the addon graph alone. Left unchanged. |
| 34315 | Old level-100 Gearing Up has Prev=Next=self | Not the user's custom ship implementation. Deliberate retirement/gating remains possible; unchanged. |
| 38923, 39090 | Level-110 Ashran variants have self-prerequisites and no ordinary starter | Other similarly named level-100 variants exist. No evidence these IDs should be enabled; unchanged. |
| 44556 | Return to Karazhan requires disabled 44944 | Old attunement branch; no prerequisite removed and no replacement invented. |

The current [44944](https://www.wowhead.com/quest=44944/aura-of-uncertainty) and
[44556](https://www.wowhead.com/quest=44556/return-to-karazhan) pages mark them
obsolete. A current retail label alone does not establish the exact 7.3.5
transition. Contemporary reporting also describes attunement changes around 7.3:
[Wowhead's September 2017 tracker update](https://www.wowhead.com/forever/news/legion-attunement-tracker-updated-for-7-3-requirements-271427).
No version-specific replacement was established in this pass. Locally, map
1651's heroic and mythic access requirements have quest_done_A/H=0, and quests
44685/44686/44694 do not require 44556 through addon prerequisites. Thus this
dead introductory branch is not evidence that the dungeon itself is locked.
Travel, phasing and full dungeon playability remain separate checks.

## Method and limits

The graph models both explicit PrevQuestID and incoming NextQuestID edges,
including negative IDs. Alternative prerequisites are OR alternatives, matching
`Player::SatisfyQuestPreviousQuest`; a simple positive-PrevQuest-only cycle scan
produces false positives. Disabled quests are excluded as ordinary entry points.
Existing character reward histories and scripts which grant quests directly
can change the result. The graph is intentionally optimistic about class/race,
exclusive groups, reputation, event timing and phases; reachable does not mean
playable. The direct condition check records candidates even when another
ElseGroup could provide an alternative.

This pass does not prove that every questgiver appears in every character's
phase, that every kill credit fires, or that every scenario/escort finishes.
No auto-completion workaround or bulk enabling/disabling was introduced.

## Validation and runtime

- `Test-LegionQuestChains.ps1`: production SQL executed twice against temporary
  copies of the real table; first execution changes one row, second changes
  zero. Downstream requirements, archaeology rank and unrelated links remain
  intact. A modified prerequisite prevents the guarded update.
- `test_legion_quest_audit.py`: reproduces the blocked three-quest graph before
  repair and reachable graph after it; covers an external edge opening a cycle,
  disabled roots and negative active-quest prerequisites. These test diagnostics,
  not the entire Player quest implementation.
- `Test-LegionIntroSequence.ps1`: 76 quest/phase progression checks pass.
- `Test-BrokenShoreReentry.ps1`: production login/LFG/gossip recovery tests pass.
- Migration applied to the runtime world database; fresh export confirms
  41183.NextQuestID=41184 and graph candidates decreased from 13 to 10.
- With zero online characters, worldserver was gracefully stopped and started
  from `build-extractors/bin/Release`; ports 8085/8086 are listening. The existing
  bnetserver remains on 1119. No new DBErrors entries appeared in startup.
- No C++ changes or new server build were required. Custom boost/scenario code,
  runtime configuration and extracted data were not edited.

Re-run from the repository root:

```powershell
./tools/research/Export-LegionQuestAudit.ps1
python tools/research/audit_legion_quests.py --output .codex/legion-quests/report.json
./tools/tests/Test-LegionQuestChains.ps1
python tools/tests/test_legion_quest_audit.py
```

In-client completion has not been performed in this pass.
