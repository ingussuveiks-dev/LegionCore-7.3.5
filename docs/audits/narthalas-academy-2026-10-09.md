# Nar'thalas academy: entry, lessons and Tidestone collection

Scope: ten quests, **37467, 37468, 37736, 37678, 37518, 42370, 42371,
37729, 37730 and 37469**, with 21 original objectives. The subsequent **Save
Yourself (37530)** and **The Head of the Snake (37470)** are separate, unfinished
work. This report is not a claim of a completed client playthrough.

## Evidence

The 2018-02-19 TDB 7.3.5 archive supplies the objective identities, amounts and
flags. Runtime build-26972 DB2 supplies the spells, action bar, scene packages,
scene Lua, rune origin and visual assets. File hashes and selected records are
in `narthalas-native-2026-10-09.json`. SQL StorageIndex values are retained for
existing character saves. Delivery quests 37468 and 37518 legitimately have no
objectives.

[Wanding 101](https://www.wowhead.com/quest=42370/wanding-101) describes aiming
the extra-action projectile at the classroom targets. Its native button is
212782 in OverrideSpellData 711. The existing six 117780 spawns use the same
native model (69903) as the original story target 107279; 117780 is also the
actual objective of world quest 44784. These remain separate quest credits.

[Study Hall](https://www.wowhead.com/quest=42371/study-hall-combat-research)
and [Pop Quiz](https://www.wowhead.com/quest=37729/pop-quiz-advanced-rune-drawing)
corroborate the book battles followed by rune drawing. Modern level/reward
values are not imported.

## Repairs

- **37467, The Walk of Shame:** retain native summon 177816, actor 88889 and all
  ten original waypoint positions. Replace the unconditional final timed-list
  credit with actual movement completion, owner proximity and LOS. The guide
  waits between legs for its player. A different player has an independent
  guide. Reject duplicate summons and use outside the active quest/start NPC.
  Death, abandoning, logout and map departure remove the guide; an interrupted
  run can restart at the original giver. There is a ten-minute lifetime. Remove
  only the old gossip-linked shared-giver hide/show actions, preserving its
  unrelated conversation. The walk does not restore all retail combat/dialogue.
- **37736, Dressing With Class:** link hat 239744 and wand 239745 to their
  existing loot tables. The spellbook's 238940 -> 59536 link was already valid.
  The sleeping student's 179185 -> spell loot 120948 also existed. Retain that
  native effect-157 route, adding active-quest, clicked-student, distance, LOS
  and duplicate-item checks. Use GetOriginalTarget because core target cleanup
  removes the redundant explicit unit for this self-targeted effect. Mark its
  loot quest-required. Restore the four native Flags2 values. Native on-obtain
  costume spells remain intact; their client appearance is not verified.
  Suppress only reward 179203's absent trigger 108997 at its actual launch hooks.
- **37678, Hit the Books:** original target 89834 and seven world spawns exist;
  the ordinary hostile-creature death path remains. No proximity/free credit
  is added. **37468 / 37518:** existing giver/ender delivery links remain.
- **42370, Wanding 101:** temporarily expose native action bar 711 while the
  active quest is in the lower classroom. No surviving override aura points at
  this bar, so this local player script owns the field and temporary spell.
  It does not overwrite another bar and only clears field value 711. Death,
  leaving the classroom/map, completion, abandoning and logout remove it.
  Native spell 212782 creates moving AT 7042/client entry 11511 with its existing
  visual, speed and dimensions. A local AT script checks its actual moving
  volume against the friendly dummies, which the default attackable-unit scan
  excludes. A target counts once per projectile, not once per cast/timer, and
  only for the caster. A new shot can hit that target again. World quest 44784
  uses the same button and gets its own 117780 credit. Suppress only removed
  trigger 218570, retaining the actual missile.
- **42371, Study Hall:** restore three missing book objects at their saved
  quest-POI shelf X/Y coordinates (build 22908); Z uses the existing lower
  classroom floor. Link their existing loot. The original podium referenced
  absent spell 212924 and had no script. Its new handler requires the next
  book and a nearby player, creates one personal drawing, then casts the real
  placement spell 212912/212913/212914. These supply native placement credit and
  book camera scenes 1279/1280/1281 (packages 1628/1629/1630). No creature kill is
  credited by clicking or summoning. Actual kills of Kobold, Sea Skrog and
  Senegos drawings advance the saved sequence. Remove the personal drawing's
  immunity to players, retain the existing scaling, and provide basic combat.
  Duplicate/failed summons, death and relog allow retrying the current book.
  The books stay in inventory for the original item objectives/turn-in.
- **37729, Pop Quiz:** native sparse SceneScriptText rows 13159/13160/13162 and
  13164/13165/13166 establish the actual circle game. Their origin is Path
  13470 -> PathNode 116700 -> Location 116720, not a guessed coordinate. Original
  auras 179151..179153 activate scenes 935..937. Start on acceptance, allow
  restarting the first unfinished rune through Nidriel, and advance after a
  successful scene ends. Only the exact native `Credit` event with the matching
  active aura, quest, stage and classroom location grants progress. Generic
  `complete`/cancel never grants credit. Remove these local auras on leaving,
  abandoning or logout. Correct prerequisites from Haunted Halls -> Pop Quiz
  and Study Hall -> Keys to **Study Hall -> Pop Quiz -> Keys**.
- **Completion state:** 42370/42371/37729 had SpecialFlags bit 2. In this core,
  CanCompleteQuest rejects an incomplete quest with that event flag regardless
  of its filled objectives. Remove just that stale bit; their explicit
  objectives now determine completion. Already-filled old saves are recovered
  through CanCompleteQuest, never unconditional CompleteQuest.
- **37730, The Headmistress' Keys:** Azuremoon already has a spawn, gossip-started
  fight, combat SmartAI and the correct key 120169. Its loot row's LootMode=0
  could not match normal loot mode. Restore mode 1 and quest-required loot.
- **37469, The Tidestone: Shattered:** all five shard objects and the Tidestone
  Core quest ender already existed. Restore the five missing chest-loot links
  to item 120401 and its native objective Flags2. No new shard IDs or automatic
  collection credits are introduced.

## Validation and limits

- `Test-NarthalasNative.py`: ten quests/21 TDB objectives; original item effects,
  summons, wand bar/missile, rune scene hierarchy/origin and visual assets;
  verifies absent 108997/218570/212924 and reads native sparse scene text.
- `Test-NarthalasData.ps1`: executes the three actual migrations repeatedly in
  temporary copies of the world tables, compares all 21 objectives to TDB,
  preserves saved indices/unrelated spawns/custom bindings, and checks the
  loot, event flags, actor/scene bindings and prerequisites.
- `Test-NarthalasEntry.ps1`: compiles the entire production script against world
  boundaries. Exercises ten movement arrivals, owner/range/LOS/death/logout,
  separate players, clicked-student validation, wand bar lifecycle/other bars,
  projectile intersections and duplicate hits, rune success versus cancel,
  saved stage recovery, book ownership/failed/duplicate summons and retries.
  The test supplies engine movement/volume/loot/death boundaries; it is not an
  integration simulation of navigation, DB2 dispatch or the client.
- Release scripts target compiled successfully. Deployment additionally requires
  the full worldserver Release build and a clean startup from the canonical
  `build-extractors/bin/Release` directory.

Still requires in-client validation: escort navigation, shelf heights, costume
appearance/removal, wand trajectory/hit registration and extra button, rune
scene drawing/continuation, book camera return and shared classroom behavior.
Book fights currently run as personal enemies in the classroom. The native
camera scene self-returns, but its `phase`/`phase2` callbacks and full sketch-world
phasing are not reconstructed; retail boss spell rotations are not claimed.
Save Yourself and its Farondis vehicle remain the next separate repair scope.
The custom level-100 boost, ship/Broken Shore scenario, shared mover and pet
code are unchanged.
