# Gilnean Gryphon landing crash

On 2026-11-24 the player completed the Alliance Broken Shore scenario, turned in the quest in Stormwind, and accepted the follow-up quest 40517. The server crashed after the Gilnean Gryphon set the rider down. The quest had recorded the boarding objective (100696).

## Cause

The active 12:44:36 crash report recorded `C0000005` at code section offset `0xCC3F00`. The matching Release map and disassembly locate this at `ObjectGuid::operator!=` with a null second argument. The caller at offset `0x152023F` is `WorldSession::HandleMovementOpcode`, where `movementInfo.Guid` is compared with the GUID of the session's cached mover. The gryphon script had assigned the gryphon to that slot through `SetClientControl(me, false)`, which invokes `SetMovedUnit` even on the false branch. The script allowed the server's mover pointer to survive the gryphon's removal.

World data links creature 101677 to `npc_q40517_p1` and vehicle 4511. The build 26972 DB2 records identify seat 16160 with flags `0x10800E`, with the control flag `0x800` clear. Native removal therefore has no vehicle charm to unwind. The script now uses the viewpoint API while boarded and clears only its own viewpoint on the native unboarding callback. The route, boarding objective, sound and destination speech retain their existing events. The destination callback exits the player only if this gryphon is still their vehicle.

## Verification and evidence

- Reviewed the boarding and ordinary exit callbacks, creature removal order, and player movement and viewpoint APIs. Checked the live template, native vehicle and seat records, and the matching crash binary's symbols.
- Built Release with the rebuilt `scripts.lib` as the final-link input. The linker map resolves `PassengerBoarded` to `scripts:broken_islands.obj`. The executable SHA-256 is `881668CA0D2F22C43028010F6697A20DB71A45D62D84AB8D1DAE0451AB75146F`. The compiler reported the existing C4805 at line 2387 in an unrelated script section.
- The user-led gameplay workflow uses the client's next landing to validate the repair. A separate server startup or regression suite was not run. Verify disembarkation, camera and player movement, and check for new diagnostics after the replay.
- Preserve both active crash files until the client result is known. The pre-repair executable and its map are retained locally for rollback and symbolization. No character data was changed.
