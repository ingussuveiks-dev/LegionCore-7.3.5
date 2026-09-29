# Horde Legion introduction in order

World update `2026_09_29_332_order_horde_legion_intro.sql` makes the regular
Horde introduction follow this reward order:

| Quest | Start / turn-in | Phase and purpose |
| --- | --- | --- |
| 43926 The Legion Returns | Adventure introduction / Holgar Stormaxe (4311) | Dranosh'ar preparation, 7554 / 7553 |
| 44281 To Be Prepared | Holgar / Stone Guard Mukar (113547) | Preparation and departure |
| 40518 The Battle for Broken Shore | Mukar / Eitrigg (100453) | Broken Shore scenario, then return phase 7425 |
| 40522 The Fate of the Horde | Eitrigg / Sylvanas (100866) | Orgrimmar funeral, then Sylvanas / Allari phase 7531 |
| 40760 Emissary | Sylvanas / Allari the Souleater (100873) | Meet the Illidari |
| 40607 Demons Among Us | Allari / Sylvanas | Allari reveals the demons; phase 7422 remains through 12 kills and turn-in |
| 40605 Keep Your Friends Close | Sylvanas (101035) / Elthyn Da'rai (95234) | Peaceful Sylvanas phase 6745, then the Orgrimmar plateau |
| 44663 In the Blink of an Eye | Elthyn / Emissary Auldbridge (111109) | Portal phase 7001, old Dalaran council, then Broken Isles Dalaran |

Each next quest requires the previous quest to be **rewarded**, not merely
complete. Earlier quests are hidden when a later stage is already active or
rewarded. Holgar's regular Horde skip is removed; it previously added 44663,
granted both objectives and teleported before the preceding story was done.
His Broken Shore recovery option 99 and ordinary preparation quest remain.

The level-100 ship tutorial still starts directly at 40518. It does not grant
preparation rewards. A registered PlayerScript repairs regular Horde quest
logs on login and after story rewards: it removes unrewarded later quests
whose predecessor has not been rewarded, and retires leftover preparation
once 40518 has begun. Already earned rewards, unrelated quests, Alliance
characters and demon hunters are retained.

For Maiko's recorded state (40518, 40522, 40760 and 40607 rewarded, stale 43926
and 44663 complete), the next login clears those two stale active quests and
exposes 40605 at Sylvanas. No character database edits or GM quest completion
are necessary.

## Phase transitions

- The existing funeral and Allari objective transitions remain. The demon
  hunter branches of phases 7425, 7531 and 7422 are preserved.
- The ordinary phase 7422 ends when 40607 is rewarded. Phase 6745 then keeps
  Sylvanas available until 40605 is rewarded. Both that phase and her spawn
  use visibility mask 1; her old mask 2 hid the next quest giver.
- The Orgrimmar portal phase 7001 is available while 44663 is incomplete or
  complete but not rewarded. The post-reward portal phase 7000 uses mask 3,
  retaining the normal city's mask 1 together with its portal's mask 2.
- The shared Alliance (44120) and demon hunter (41002) handoffs to 44663 keep
  their own reward requirements. The shared quest's addon predecessor is
  preserved rather than replaced with the Horde predecessor.

## Dalaran finale

The actual 7.3.5 runtime `SpellEffect.db2` defines:

1. Portal spell 228327 teleports to old Dalaran, map 0, and grants travel
   objective credit 113762. Phase 7714 / 7552 exposes Khadgar and the council.
2. Khadgar 113986's menu 20457, option 1, applies spell 227861. Aura 430
   activates scene 1449, package 1728. Its nonexistent SmartAI link 1 is
   removed without changing the aura action.
3. The scene's completion event casts teleport 230156 to map 1220 and grants
   witness credit 114506. It was missing from `spell_scene_event`. The generic
   SceneCompleted path handles both playback completion and the client's
   cinematic-skip request; accepting the quest or starting the scene grants
   no witness credit.
4. Turn in 44663 to Auldbridge in Broken Isles Dalaran.

The runtime SceneScriptText records 14546 / 14767 confirm that package 1728
is the Dalaran relocation cinematic and ends with `scene:EndScene()`.

## Validation and deployment

`tools/tests/Test-LegionIntroSequence.ps1` compiles the production quest-log
repair helper and exercises the actual migration twice on connection-local
table copies. It checks 76 quest/phase cases, including every acceptance,
completion before turn-in, reward boundary, the funeral, boost entry, legacy
skip, both Orgrimmar portals, old Dalaran and shared routes. It also checks
NPC masks, preserved Allari actions and Dalaran scene / teleport data.

`tools/tests/Test-BrokenShoreReentry.ps1` verifies the existing recovery,
solo LFG persistence and scenario-entrance behavior.

Apply world update 332 and use the rebuilt Release executable from
`build-extractors/bin/Release`. Earlier fixes through update 331 remain in
place; update 332 supersedes its regular post-reward attack-phase branch.
The server is left stopped. Full client verification of quest menus, funeral,
Allari's reveal, combat, Sylvanas's handoff and the Dalaran cinematic is still
required; passing these server/data checks does not verify client visuals.
