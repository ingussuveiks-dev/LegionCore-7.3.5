# Custom boost tutorial flow (7.3.5)

## Implemented behavior

- Ship origins use WorldSafeLocs 5219/5752. NPC offsets, the short bird flight,
  surrender at 15% health and wave timing are CUSTOM approximations, not
  Blizzard sniff data. The source explicitly marks this distinction.
- Actor offsets now use each faction's starting facing: X is forward and Y
  is left, rotated into transport coordinates. Both factions use the same
  lesson layout, with their own WorldSafeLoc deck height. Dummy (+5,-8),
  sparring (+4,+/-7), and Legion ground attackers (+2..10,+/-8; infernal +3,+4)
  stay near the trainer's lesson area instead of using negative X offsets
  that placed the Horde dummy inside the cabin. Trainer, entourage and exit
  placement also use this faction-relative frame. These are custom placements;
  deck geometry and line-of-sight still require visual tests on both ships.
- Every fresh tutorial instance restores the player to that faction's
  transport-local WorldSafeLoc pose before sending the scenario. A stale
  position and facing saved while logging out aboard the ship are replaced.
- An unfinished world/instance socket handshake is closed after 10 seconds.
  This lets the 7.3.5 client's built-in WorldAttempt2..5 path retry instead of
  leaving the loading bar blocked until the general 15-minute socket timeout.
- The tutorial action bar is a packet overlay. It starts empty; active known
  spells from CAST_SPELL criteria in completed/current DB2 lessons accumulate
  as lessons advance. Form/stance pages also receive the lesson buttons.
- Persistent action buttons are never cleared. Client button edits on the two
  tutorial maps are ignored. Spell learning and reconnects regenerate the
  overlay. Leaving either map sends the saved normal layout again.
- Departure also fills vacant normal slots with any newly learned active class
  spells. Macros and occupied slots are preserved; passive/profession/mount
  spells are filtered by the existing boost action-bar helper.
- Sparring opponents yield before lethal damage, become friendly/nonattackable,
  kneel and credit event 48772 once. Legion attackers remain lethal encounters.
- Training actors follow the recorded Warlock sequence: no dummy during the
  initial conversation or pet-summoning lesson, one persistent dummy for the
  target-based lessons, one sparring opponent, two sparring opponents, the
  Legion wave, and finally the exit bird. Completed sparring waves are removed
  before the next wave. Other class scenarios delay their dummy until their
  opening pet/form/preparation lessons are complete.
- The shipboard dummy is rooted so Fear can satisfy its lesson without sending
  the dummy running off the moving deck. Warlock sparring NPCs appear at steps
  10 and 11, after the Fear and greater-demon lessons.
- The boost dummy uses the training-dummy unit type. Instance combat cleanup
  drops expired dummy-only hostile references, allowing pets to leave combat
  after their attack is stopped without dropping real encounter references.
- The trainer explicitly advances the opening faction gate after casting
  219615. Client DB2 places the mutually exclusive Horde and Alliance criteria
  below an ALL node, so the generic evaluator otherwise leaves the visible
  faction objective at 1/1 without advancing to the first class lesson.
- Scripted scenario transitions add the destination step to `ActiveSteps`
  before sending `ScenarioState`. The old order sent a new `CurrentStep` with
  only the previous step active, causing the 7.3.5 client to disconnect with
  reason 16 immediately after the trainer response.
- The exit bird is spawned only for the final departure stage. Departure
  reuses that bird with vehicle layout 4933, detaches it and its rider from
  the gunship, waits for boarding to finish, then flies in world coordinates.
- Spell 219912 is cast by the bird with the player in seat 0. Its DB2 passenger
  targeting triggers 227058, bound to the existing Broken Shore queue script.
  The appropriate Battle for the Broken Shore quest (40518/42740) is offered
  directly if absent and there is room in the quest log.
- If boarding fails or the queue has not transferred the rider after 15 seconds,
  the custom recovery exits the vehicle and returns to the faction capital.
  This is NOT a claim of an exact retail departure sequence.
- Completed scenario updates are bounds-checked. A directly constructed tutorial
  has no LFG dungeonData; reward handling now checks that pointer before use.

## Verification on 2026-09-23

- Release worldserver and bnetserver build succeeded in the canonical
  `build-extractors/bin/Release` directory. The first rebuild exposed an invalid
  Creature::Say overload; this was corrected and rebuilt successfully.
- worldserver loaded the installed MariaDB databases and reached `ready...`
  (world initialization: 32 seconds). The initial claim of clean shutdown was
  incorrect: a native crash report at 08:48:29 showed an access violation after
  the database-close log lines. See `shutdown-crash-20260923.md` for the later
  diagnosis, fix and verified shutdown tests.
- Read-only legion_world checks confirmed both instance scripts, both trainers,
  both sparring scripts, both exit scripts, vehicle 4933 on the Horde exit
  template, the 227058 binding and the four relevant introduction quest rows.
- `git diff --check` passed. Both server processes were left stopped.

This is a build/startup check, NOT an in-client end-to-end gameplay test.
Exact vehicle animation, movement on the moving deck, every class/spec lesson,
and the Broken Shore queue still require client verification. The DB2 lessons
target particular training specializations; unsupported spell replacements or
specializations are not silently learned or automatically marked complete.

## Client acceptance checklist

1. On both factions, enter the ship and verify the main bar is empty. Talk to
   the trainer; verify the first required spell appears, then each next spell.
2. Relog during a lesson. The current loaded instance should resend its stage
   and bar without duplicate NPCs. A new/unloaded instance restarts training;
   lesson progress is not persisted across instance destruction/server restart.
3. Complete one sparring opponent, then two. Test pet damage and a lethal hit:
   each opponent must kneel, not die, and award credit exactly once.
4. Complete the Legion defense. Board the bird and verify takeoff, quest grant,
   transfer into Broken Shore, and restoration of the normal spell layout.
5. Test early skip and unavailable queue: the capital recovery must restore the
   bar and leave a playable character able to continue the Legion introduction.
6. Confirm existing macros/custom buttons survive both normal exit and relog.
   Check druid form/warrior stance pages separately.

## Follow-up on 2026-09-24: waves and landing

- The 13:26–13:27 test still logged all three sparring deaths through the
  fallback. The instance damage hook now runs after damage scaling and compares
  target-relative health, matching the core death check. Compile-time checks
  cover lethal hits, the 15% boundary, zero damage and 64-bit health.
- The second sparring wave waits 2 seconds after the first opponent yields.
  The Legion wave assigns 12 attackers round-robin to the player, trainer and
  four entourage NPCs. Allies use the player's faction; untagged ally kills
  bypass the individual-only achievement filter so scenario kills count.
- The original gossip bird is detached and reused. Flight waits for both the
  vehicle attachment and boarding spline; a duplicate bird is no longer spawned.
- Broken Shore scene `port`, `complete` and aura 217781 share one idempotent
  landing routine. A server check also lands passengers when their ship reaches
  the shore, disappears, or its introduction exceeds 60 seconds. Old scene
  callbacks cannot reattach them or restore the cinematic viewpoint afterward.
- This custom landing places players beside existing first-beach allies:
  Alliance NPC 90717 at (486.929, 2052.26), Horde NPC 90708 at (567.826, 1886.94).
  Positions were checked against local `legion_world.creature` data. The old
  199358/225152 destinations are on ships, not the beach itself.

Client acceptance remains required: repeat the full sequence, check the 2-second
gap and two independent surrenders, watch ally combat, then board and remain idle
through arrival. Expected result is a character on the beach with no transport
attachment. The `died before surrender` fallback must not appear. Server startup
and gameplay were not performed for this revision because the user requested
that the server remain stopped.

## Follow-up after the 14:42 test on 2026-09-24

The client test disproved that the previous surrender fix alone was sufficient.
The two-second delay worked, but all three opponents surrendered with just
2 target-relative health and final damage 2246855168. Templates 109010 and 111995
are level 100 with HpMulti 1.5, yet their scaling rows contain MinLevel=0,
MaxLevel=0 and Duration=100. `HasScalableLevels` previously tested only whether
the row existed. `GetLevelForTarget` therefore clamped targets to level zero,
and health/damage normalization used level-zero stats.

`HasScalableLevels` now validates the range before enabling scaling. Duration-only
rows use ordinary template stats; valid 100..110 scaling is retained. This
corrects the same malformed-range behavior for other templates too (the local
database contained 9974 zero-range rows). `CreatureScalingTest.cpp` covers zero,
partial, reversed, overflowing and valid ranges. Sparring summons are independent
NPCs and accept progress-producing damage only from the trainee or their pets.
Spawn and surrender diagnostics include effective health and damage source.

The server logged successful boarding at 14:43:59, but the client camera remained
on deck. The departure now explicitly synchronizes passenger movement and sets
the bird as viewpoint after boarding; dismount and the map-transfer spell clear
that viewpoint. Vehicle 4933 uses seat 16967 (flags 0x40100003, no CAN_CONTROL),
so the control-seat charm path is not establishing the view.

The level-range regression test and Release compilation passed. A repeat client
test is still required for actual hit counts, wave timing and camera motion.
