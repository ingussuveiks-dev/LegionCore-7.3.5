# Custom boost tutorial flow (7.3.5)

## Implemented behavior

- Ship origins use WorldSafeLocs 5219/5752. NPC offsets, the short bird flight,
  surrender at 15% health and wave timing are CUSTOM approximations, not
  Blizzard sniff data. The source explicitly marks this distinction.
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
- The exit bird is available from the beginning as a skip/recovery route.
  Departure creates an individual vehicle using vehicle layout 4933, removes
  it and its rider from the gunship, and flies away in world coordinates.
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
  (world initialization: 32 seconds), then shut down cleanly. No ERROR/FATAL or
  missing tutorial spell/script messages appeared in that startup log.
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
