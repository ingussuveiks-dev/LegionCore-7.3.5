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

### Dalaran arrival ordering, 2026-10-12

The portal landing update 2026_10_12_11 alone was insufficient: the client
retest saved Maiko at Z=707.601, below the measured street floor Z=747.131.
Dalaran over Deadwind Pass is static transport 246854 (spawn 129023), with
GO_FLAG_MAP_OBJECT. The old map initialization sent the active player first
and the city only in a subsequent visibility update. The native 26972
TransportAnimation/TransportRotation tables contain no path for this entry.

`Map::SendInitSelf` now puts visible map-object static transports before the
player in the same update packet. Initial client visibility is cleared before
this packet, preserving the city's known GUID and avoiding a duplicate create
in the following visibility update. Normal visibility rules still apply;
other phases, despawned objects and ordinary elevators are not forced visible.
The portal coordinates, quest credit and cinematic destination are unchanged.

`Test-MapObjectArrival.ps1` compiles the production initialization handler and
checks packet order, visibility tracking, excluded objects and existing boat
passengers. It failed against the old handler and passes with this change.
`Test-DalaranPortalLanding.ps1` and all 76 intro data checks also pass. These
prove the server ordering and data, not client collision timing: repeat the
portal in the 7.3.5 client and confirm both standing and walking beside Khadgar
before calling the reported fall resolved.

The subsequent flight-mode retest established that sending the city first is
insufficient: the client shows empty space for several seconds, then streams
the walls and floor. Disabling flight after that leaves the player standing
normally. The native client WMO also contains an upward-facing collision face
at Z=747.1306 (MOPY 0x08), matching the saved standing height 747.131.

Arrivals within 75 units of city object 246854 on map 0 now start with gravity
disabled and movement rooted in their initial self-create packet. This is
scoped to the council-room arrival area. The hold ends eight seconds after
the matching loading-screen-close notification, allowing dynamic WMO streaming
to finish after the static map loading screen. This is a fixed grace period,
not a collision-ready acknowledgement; unusually slow clients still require
testing. A missing notification expires after 30 seconds. Logout, map removal,
death and vehicle/transport transitions clear the owned hold. Existing flight,
levitation, roots and vehicle passengers retain their own movement behavior.

`Test-MapObjectArrival.ps1` also covers the protected first self-create, slow
loading, duplicate/wrong-map/missing notifications, cleanup and existing flight
or roots. Retest the portal with GM flight **off before entering**: wait for
the short hold to end, walk on the floor and then speak to Khadgar. The cinematic
and quest credits are unchanged. The council's explicit levitation movement
templates are preserved; their visual intent has not been independently
verified against a retail recording.

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

## Funeral replay report, 2026-10-12

Maiko reported that the movie before arriving at 1221.57, -4450.17, 23.78
repeated until manually skipped. Native spell 198897 plays movie 474, and
the existing world linked-spell row casts teleport/credit spell 200282.
The NPC also cast 200282 immediately before starting the movie. That
redundant call is removed so the single linked teleport observes the
movie's delayed-teleport state. The NPC now checks objective index 2
(100934) both before its conversation and after the 45-second delay, so
an already credited, still-active quest cannot replay the movie through
a fresh NPC event. The delay and native three-second credit aura 200252
are retained.

`Test-HordeFuneral.ps1` compiles the production eligibility/start helpers,
movie sender and completion handler. It checks the single deferred
teleport, normal completion/skip, repeated callbacks and already credited
quests. Linked-spell dispatch is simulated using the verified live row;
this does not test client video playback. The existing log had no movie
start/finish records, so the precise cause of the reported endless playback
remains unconfirmed. New movie start and completion/skip log messages make
the next client reproduction observable. Do not call the client loop fixed
until that run finishes without manually skipping.

The equipped-item audit and related Streten bracer lifecycle correction are
recorded in `audits/maiko-legendaries-funeral-2026-10-12.json`.

The follow-up report clarified that playback restarted before reaching the
end. `Player::SendMovieStart` now rejects movie 474 if a movie is already
being watched and records the suppressed overlap. This also covers a caller
outside the funeral NPC's quest guard. `Test-HordeFuneral.ps1` checks this
second barrier and permits a new explicit playback after completion/skip.
Maiko was reset again to Eitrigg with 40518 rewarded and 40522 not started;
the server is left stopped for the user's visible launch and client retest.
See `audits/legendary-defensive-effects-2026-10-12.json` for the accompanying
Norgannon timing and absorption-bypass fixes and validation limits.
