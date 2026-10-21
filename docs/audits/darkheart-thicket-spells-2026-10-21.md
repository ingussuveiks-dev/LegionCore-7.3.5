# Darkheart Thicket spell and ground-effect audit

Scope: Legion 7.3.5.26972, map 1466, all 31 distinct templates spawned in the
world database, four boss implementations, their summoned helpers, native
spell effects, and the corresponding area-action data. The reported run was
Normal difficulty. No character quests, equipment or dungeon completion were
changed. This is a code/data audit and regression result, **not a client-verified
full dungeon clear**. Existing file comments claiming "100%" are not evidence.

Follow-up migrations 09/10 and the additional native tests below address the
remaining identified code gaps: Dryad escape, Burster regeneration, skipped HP
windows, toxin attribution and Oakheart awakening. Earlier findings below are
retained as the historical baseline; the follow-up section supersedes their
"unimplemented/open" implementation status. Full client traversal is pending.

## Reported flowers and the actual visual chain

The missing effects were not explained by a graphics setting alone:

* Keeper 99359 and Dryad 99358 had no spell AI. Blossom 99360 only had its
  21–22 second Root Burst SmartAI timer; a short fight could end before it cast.
* Keeper's 198799 area dummy had no handler to launch its native 220369 missile.
  That missile triggers 198910 and summons mushroom 100960. Native target limits
  are one on Normal and three on Mythic. The helper had no model/area/burst AI.
* Root Burst 201123 had damage/knockback but no warning-helper creation. Native
  201129 summons 101958 for 4500 ms; 201145 is its 3000 ms warning. Native
  201123's cast is also 3000 ms and its explicit target is ANY unit. The flower
  now summons the warning at a random Player, then casts at that stationary
  helper. This avoids following a moving player with the damage center or
  warning under a pet tank. A failed summon does not produce an invisible hit;
  interrupting the flower cannot cause the helper to deal independent damage.
* Kudzilla's Noxious Spit 204970 referenced missing native spell 204972. The
  surviving chain is 204973 missile -> 204974 impact -> area 6078 -> aura 204976.
  Its 20-second pool had no damage/slow actions either. The new aura callback
  retains the native 1000 ms ticks and sends one missile to a random Player per
  tick; this fallback targeting is a reconstruction, not a retail packet trace.
  The live hotfix database has no 204972 spell or overriding effect/misc/target
  records for the checked contracts; this is not merely a missing base-file name.

Root Burst is a brief ground warning followed by an eruption, not a permanent
green pool. Keeper mushrooms are separate visible objects. Rotting Earth and
Noxious Spit are actual persistent areas. Thus different expected "puddles"
come from different creatures.

`darkheart-visuals-2026-10-21.json` records the native visual/event/kit/model
attachment chain and SHA256 values. Four referenced M2 assets were fully read
through read-only CASC access, then their names and animations decoded:

| Spell | Visual / kit | Main model and animation evidence |
| --- | --- | --- |
| 198937 mushroom model aura | 51690 / 62054, aura events 7–8 | EffectName 22508 -> FileDataID 1037392, `Highmaul_FungalGiant_Mushroom_Mind`; attachment 127 explicitly uses Hold 158 and end 159. Model contains Stand 1967 ms, Hold 10000 ms, end 1000 ms. |
| 201145 root warning | 52393 / 63089, aura events 7–8 | EffectName 24740 -> 1034149, `Target_Earth_State_CenterOnly_Projected`; Stand/Hold/end sequences exist. |
| 198916 burst | 51680 / 62037 | 1006478, `Podling_Impact_AreaSplash`, Stand 2000 ms. |
| 220369 missile cast | 51688 / 62050 | 166602, `Nature_Cast_Hand`, attachments 1/2. |

Two ancillary mushroom attachments reference EffectName 24054, absent from the
local native table. The main mushroom model exists. This unresolved auxiliary
reference is retained in the report; the audit does not claim every particle
or the client's rendering is verified. EffectName decoding uses the actual
7.3.5 layout B930A934 rather than the later schema in this core's unused loader.
No client asset or global DB2 structure was modified.

The [original Blizzard hotfix archive](https://worldofwarcraft.blizzard.com/en-us/news/20252902/hotfixes-september-14)
also explicitly identifies Rotheart Keeper's Vile Mushrooms as exploding
hazards. The [Ashamane implementation](https://github.com/AshamaneProject/AshamaneCore/blob/b09c1c393cf2e01bc5e857ea6117680d2aaeadb0/src/server/scripts/BrokenIsles/DarkheartThicket/darkheart_thicket.cpp)
was a code cross-check for mushroom model/area spells, charge/blood impact
actions and completion-only Tormenting Eye fear, not proof of retail timings.

## Implemented repairs

Migration 06 and `darkheart_thicket_trash.cpp` restore Keeper, mushroom, warning
and Blossom execution. Mushroom arming is 3 seconds, followed by Player
proximity/expiry detection and one guarded burst; reset/death removes owned
helpers without an extra cleanup explosion. The flower retains its existing
21–22 second schedule and stationary combat behavior. Mythic-only infusion
225497 enables native self-heal 225503 on flowers within 25 yards, accounting
for multiple keepers and removing healing when the last one dies or moves away.

Dryad now throws 198723 and poison spear 198904. Razorbeak faces a Player before
the linked forward charge 200768. Deadeye uses native 201858 Shoot, 201902
Scorching Shot and 201885 Shadow Decoy. The unrelated same-named rocket spell
203948 is **not** used. Slime has triggered death spell 201191; Fury's existing
Dispersion death cast is explicitly triggered. New combat intervals for formerly
empty AI are conservative script timers, not sniff-verified retail schedules.

Native area actions restore charge impact 200771, Blood Assault 201227, Rotting
Earth 200822, abomination trail 198408 and Noxious Spit 204976. Admission is for
attackable Players. Persistent aura cleanup uses owner-specific native removal;
it does not remove another caster's aura. Tormenting Eye 204243 only fears with
204246 on natural expiry, preserving interrupts.

Migration 07 closes additional boss paths found during the sweep:

* Mythic Nightfall missile 212464's original impact only created an area; the
  additional summon 198432 was not invoked. Its new cast callback is restricted
  to Glaidalis 96512 and Mythic/Keystone and creates the abomination. Abominations
  now emit native trail 198464 every second. Summon happens at missile launch;
  exact retail missile/add timing remains a client/timing check.
* Down Draft 5529/6084/8722 had three or four conflicting template rows each.
  The loader keys by entry alone and appends generated wind actions for each
  wind-bearing row, so this was not merely harmless duplicate SQL. There is now
  one complete definition per difficulty, using existing valid visual 51790,
  custom entry 10771, 50-yard radius and native -7 wind. Native damage 220855 and
  cleanup apply on all three entries. The dispatch intentionally makes this
  caster-targeted damage a Player self-cast; the regression executes that branch.
* Heroic Growing Paranoia area 8082 lacked Normal 5631's touch action. It now
  uses the same native 200329 action. Natural expiry of 200289 self-casts
  Waking Nightmare 200243; early fear/removal does not create the isolation aura.

## Complete spawned-template inventory

"Present" below means the named executable path/data exists, not client proof.
The damage/duration/radius values remain native difficulty-specific values.

| Entry / creature | Spell path and audit result |
| --- | --- |
| 95766 Crazed Razorbeak | Empty AI repaired: 200768 -> 200769/200770, area 5670 -> 200771. |
| 95769 Mindshattered Screecher | SmartAI 200630 Unnerving Screech present. |
| 95771 Dreadsoul Ruiner | 200642 Despair and 200658 Star Shower -> configured 204383 dummy trigger present; OOC 200448 visual retained. |
| 95772 Frenzied Nightclaw | 200618 leap -> native 200620 / Mythic 225484, including existing bleed/heal-threshold script. |
| 95779 Festerhide Grizzly | 200580 Maddening Roar and 200589 Festering Swipe present. Existing one-shot roar health window 60–65% can be skipped by a large hit; retained as an open timer/trigger issue. |
| 96512 Archdruid Glaidalis | 196346 leap/farthest-player script, 196376 tear, 198360/198376/198379 rampage and area 5443/198386 present. Nightfall 198400 -> native difficulty missile -> 198402/5445 present; missing Mythic add/trail paths repaired above. |
| 99192 Shade of Xavius | 200182 Festering Rip, 204808 -> 200185 Nightmare Bolt, 200238 Feed on the Weak, 200359 Paranoia and 200050 final phase present. Paranoia/isolated follow-up gaps repaired above. Malfurion release was already implemented. |
| 99200 Dresaron | 199329 -> 199332 stalker -> 191325 breath present; 199389 -> 199458/5542/199460 falling rocks plus 218587 damage present. Down Draft duplicates and missing difficulty actions repaired above. |
| 99358 Rotheart Dryad | Empty AI repaired for spears. Retreat/jump behavior remains unimplemented; exact native spell-to-NPC mapping was not established, so no guessed unrelated jump was installed. |
| 99359 Rotheart Keeper | Missing mushroom AI/missile chain repaired; infusion is Mythic only. |
| 99360 Vilethorn Blossom | Warning, fixed-location burst and infusion lifecycle repaired. First burst still waits 21–22 sec. |
| 99362 Kudzilla | 204967 Barbed Bite/native bleed present. 204970 Spit missing-trigger/pool repaired. |
| 99365 Taintheart Stalker | 201517 Dark Hunt and native movement/continuation present; exact two-target choreography still needs client observation. |
| 99366 Taintheart Summoner | SmartAI 201837 Shadow Bolt and 201839 Curse of Isolation -> 201842 present. |
| 100526 Tormented Bloodseeker | 201361 Darksoul Bite -> 201365 native drain path present. |
| 100527 Dreadfire Imp | 201411 Firebolt, 201399 -> 201400 Dread Inferno present. Existing 49–51% one-shot HP window and NOT_TOP target can skip Inferno on burst damage or a sole Player threat target; left open. |
| 100529 Hatespawn Slime | Empty spell AI repaired with death burst 201191. |
| 100531 Bloodtainted Fury | 201226 assault area repaired; 201272 -> 201273 blood bomb present; death 201250 -> four 201257/201251 summons made triggered. |
| 100539 Taintheart Deadeye | Empty AI repaired with Shoot/Scorching Shot/Shadow Decoy. Native decoy 102277 and teleport 201892 exist; clone appearance/choreography unverified. |
| 101660 Rage Rot | 200817 Rotting Earth area actions repaired; 200826 Viny Grasp present. |
| 101679 Dreadsoul Poisoner | 200666 form, 200684 toxin and existing 200686 eruption/200687 hallucination chain present. Existing removal script finds a nearby living poisoner rather than the original caster; death/multiple-caster behavior remains an open issue. |
| 101991 Nightmare Dweller | 204243 completion fear repaired. Existing death 204852 is a nearby-entry dummy without a configured trigger; Oakheart also has no five-dweller unlock gate. The intended release sequence is not certified. |
| 103344 Oakheart | 204666 Shattered Earth, 204574 -> 199051 -> 199060 -> root 100991 -> 199067/6037/199063, 204667 breath, 204611 vehicle grip/204644/204651 throw and non-Normal 212786 Uproot paths present. Actual passenger/throw behavior needs client testing. |
| 100533 / 101072 corrupted eggs | Existing encounter-gated Player proximity hatching, 199304/199313 and whelpling AI retained. |
| 541898 custom Emerald Egg | Existing achievement egg -> 220921 / 111008 path retained; this custom entry is not proof of retail placement. |
| 100400 dead preservers | Corpses, not combat spell actors. |
| 100403 druidic preservers | Glaidalis introduction actors/197642 and scripted deaths. |
| 100404 invisible preservers | Introduction visual targets, not ordinary combat actors. |
| 102851 invisible shield | Introduction barrier actor; boss intro removes 203257. |
| 108460 injured preservers | Existing spellclick/quest interaction retained; not ordinary trash combat. |

Summoned-only review additionally found Bloodtainted Burster 100532 has no spell
AI. Native 225562 Blood Metamorphosis summons Fury 113398, but this regeneration
chain was not implemented or declared working. Hallucinated spider 101756 and
decoy 102277 likewise have no custom spell AI; default melee/illusion behavior
alone is not proof of full encounter behavior. These findings mean the answer
to "does every spell/mechanic work?" is still **no**; the ground-effect repairs
do not erase these separately recorded gaps.

## Automated evidence and remaining client checks

`tools/tests/Test-DarkheartTrash.ps1` executes extracted production AI and aura
callbacks: arming, expiry, one-shot/reentrant burst, cleanup, failed summon,
stationary root target, player selection with pet tank, infusion overlap and
difficulty, interrupted/expired fear, Spit fallback, Mythic summon gates,
abomination trail timer and expired-versus-early Paranoia transitions.

It replays migrations twice on connection-local shadow tables, preserves
unrelated rows, checks single Down Draft definitions and dispatches **14 actual
SQL area actions** through extracted native attack/player/self-caster/cast and
owner-specific removal branches. Native C++ DB2 reads check target kinds,
target counts, cast times, model visual IDs and missile/summon contracts.
`verify_darkheart_trash_release.py` checks deployed PE instructions and loader
dispatch, resolving incremental-link thunks rather than matching strings.
The real dummy spell-ID selection and option-11 missile dispatch are also
executed, confirming that SQL trigger 0 uses native base points 220369 and
retains the Keeper as caster and the selected Player as target.

Fixtures do not execute the complete world update, client targeting packets,
spell rendering, every boss cast or collision/pathing. Required client checks:
allow a Blossom to live beyond 22 sec, observe/leave its 3-sec warning, interrupt
it and confirm no hit; keep a Keeper alive beyond its first cast, observe and
disturb a mushroom, then test wipe/cleanup; check Rage Rot/Kudzilla pools by
entering/leaving them; verify Down Draft wind release and damage on all three
difficulties, Mythic add/trail cleanup, Paranoia touch versus expiry, Oakheart
roots/grip/throw and the complete dungeon. Do not infer these from startup.

## Release deployment

Compiled the original generated `scripts/Release/scripts.lib`, then linked the
staged worldserver against the original final-link dependency paths. Copied
only `worldserver.exe` and its matching MAP into `build-extractors/bin/Release`;
configuration, certificates, DLLs, extracted data and logs were preserved.
Scoped pre-migration world rows and the previous executable/MAP were backed up
privately. Both migrations are applied to the live world database.

Canonical executable SHA256:
`488805384531425548ec5ace7823eb945635f190609238a54a97aaac192c4fe9`.
The instruction-level check passes for 13 Darkheart handlers and the actual
Legion loader call. `Test-ScepterLogin.ps1 -VerifyReleaseBinary` also passes.

No user-owned process was stopped. The server was absent before deployment.
Owned hidden validation PID 31196 reached ready at 16:15:13 local time and
shut down gracefully by 16:15:17 with exit 0. No new Server/DBErrors/stderr
errors were emitted. The first readiness check incorrectly rejected existing
DBErrors from the user's earlier 15:01/15:28 runs (230986/186608 spellclick
vehicle warnings); its second check compares only newly appended records and
preserves those historical messages. These old warnings are separate findings,
not a clean bill of health for every in-game interaction. No validation server
is left running. Client dungeon traversal remains pending.

## Follow-up: remaining trash and Oakheart lifecycle

Dryad's native movement leaf is **Prancing Escape 198792**, not one of the
unrelated Disengage spells. Native effect 149 uses caster-back target 48,
destination scatter 86 and a 15-yard radius. Migration 09 faces the current
victim, including a pet tank, before a close-range (0..8 yards) cast. The native
destination selection and charge retain collision checks. The 12..15-second
repeat interval is a reconstruction, not a retail packet timing measurement.
The unused 198722 dummy/198759 targeting helper is not presented as a complete
retail target-selection implementation; the actual backward movement leaf is
executed directly. Final trajectory, terrain clipping and creature animation
remain client checks.

Bloodtainted Burster 100532 now casts native Bloodbolt 201298 at a random Player.
On Mythic/Keystone at or below 50% HP it attempts native, interruptible
three-second Blood Metamorphosis 225562. Only its actual summon callback for
113398 replaces the Burster, retaining live threat amounts and the current tank.
Failed/interrupted casts can retry, and stale/wrong/repeated callbacks cannot
consume the transition. The separate regenerated Fury receives Blood Assault
and Blood Bomb, with **no Dispersion death action**, avoiding recursive splitting.
The initial/repeat/retry timers (1/4.5/6 seconds) are script scheduling choices;
they are not claimed to be sniff-verified. Normal/Heroic never attempt this
Mythic-only transformation. Native damage, leech, summon and cast data are used.

Grizzly's one-shot roar now admits all HP at/below 65%, and Imp's one-shot
Inferno all HP at/below 50%; a large nonlethal hit cannot jump over their former
60..65% and 49..51% windows. The Imp selects a Player without excluding top
threat, including the solo-player case. Grizzly Swipe also remains available
below its former 15% floor. Existing one-shot flags and repeat timings remain.

Nightmare Toxin now uses its original aura caster, including a still-present dead
poisoner, rather than finding an unrelated living poisoner within 40 yards.
Eruption follows the native tooltip's natural-expiry contract; dispel, death and
other early cleanup do not erupt. A missing original caster never substitutes a
Player or another enemy. This avoids changing area hostility/faction. Native
200686 retains the area damage and its eight 200687 spider summons. Spider
illusion motion and Decoy clone/teleport rendering remain unverified client
behavior; no additional missing named spell path was established for them.

Oakheart starts passive, protected and in native Deep Roots 165950. Five distinct
database Nightmare Dweller deaths unlock him; pet kills use the same instance
death callback. Progress is keyed by DB spawn ID, so repeated notifications and
corpse/grid reloads cannot double count. The versioned `D1` save suffix preserves
partial progress across a server restart. Old boss-only completed saves remain
unlocked; malformed/duplicate/zero-ID suffixes fail closed. A late-loaded boss
reads the completed gate, and an encounter wipe does not restore killed trash.
The optional 204852 death visual resolves its native nearby-entry target to
Oakheart via source-13 conditions. Essential awakening does not depend on that
cast succeeding. Cancel Deep Roots 165953, direct scoped aura removal and scoped
flag cleanup release the boss without removing unrelated flags.

`Test-DarkheartRemaining.ps1` executes the extracted Burster, toxin, instance
death/save/load and boss reset/LOS handlers. Its migrated SQL rows also execute
the actual SmartAI HP/range/player-selection branches and native backward
destination/charge branches. Tests cover difficulty, pet targets, nonlethal HP
skips, interrupted/failed summons, duplicate callbacks, original-caster death,
deferred boss creation, partial/legacy/malformed saves and cleanup after a wipe.
Migrations replay twice on connection-local shadow tables; unrelated template
and SmartAI rows are preserved. Native C++ DB2 reads verify the actual 7.3.5
spell IDs, three-second casts, leech, stun/removal, 113398 summon and 15-yard
escape radius. Live effect/misc/target hotfix override counts are zero for the
eight checked spell contracts. Fixtures do not execute full collision geometry,
spell packets or client rendering.

`darkheart-followup-visuals-2026-10-21.json` traces native SpellXSpellVisual,
events, kits, model attachments and M2 animation lists for the new paths. All ten
referenced models were fully read from the 26972 CASC installation and all 32
index hashes stayed unchanged. Metamorphosis visual 60753 uses kits 74599/74600
and Nightmare hand effects; Awakening Roots 53647 uses kit 64877 and eye
attachment 19. No referenced EffectName is missing in this new subset. Prancing
Escape has no separate SpellXSpellVisual row; native movement carries its
presentation. None of this establishes that the client played the effects.

The prior trash/14-area-action fixtures and handshake deadline/retry fixtures
also passed again. `verify_darkheart_remaining_release.py` checks the existing
13 handlers plus six new linked handler contracts, actual instance death/grid
and boss reset callers, and the original-caster toxin call path. Release build,
canonical deployment and startup evidence are recorded below after completion.

Follow-up deployment completed into `build-extractors/bin/Release`, preserving
runtime configuration, DLLs, certificates, extracted data and historical logs.
The former EXE/MAP and scoped world rows were backed up privately before applying
09/10. The first MySQL 8 dump attempt rejected the older server's missing
COLUMN_STATISTICS table; no migration had run yet. Retrying with column
statistics disabled completed all three scoped backups before either migration.

The scripts library was rebuilt in its original generated Release path. The
executable was relinked against the current original-path game/scripts libraries
(only the executable output directory was overridden). Final-link SHA256 inputs:
game.lib `30565a8cbe2d3db2ad87be12bc65f4cc337521d04dbf9a8c5ee18cff1e5e2fb1`,
scripts.lib `4f033e3cfcdc755d29a150ce7dadd973e961ad399adb76d7e204c61e98706d79`.
Canonical executable SHA256 is
`075f4260abddf6dda84b694401c48f800ef2eeed3ef945e72f8742dab66af834`.
Instruction-level checks passed for the six follow-up and 13 earlier DHT paths,
pet follow/queue/accessory changes and all five handshake diagnostics. The
canonical Scepter login binary test and Warlock regressions also passed.

No user-owned server was running at deployment. Owned hidden validation PID
30904 reached ready at 17:52:59 local time and gracefully finished database
shutdown at 17:53:03 with exit 0. Fresh Server.log had no ERROR-level records;
fresh DBErrors and stderr were empty. The validation server was stopped and no
background server was left behind. This loader/startup result establishes
acceptance of the migration/script bindings, not client encounter execution.
