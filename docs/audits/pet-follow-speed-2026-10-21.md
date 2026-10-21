# Pet following speed and destination

The user reported a permanent Warlock pet falling far behind during ordinary
on-foot running, not only Burning Rush or mounted travel. Read-only inspection
found the expected Voidwalker 1860 template speed_run 1.14286. Permanent pets
select PetAI before the template's temporary-Grimoire script. No pet record,
template speed, action bar, GUID or ownership field was modified.

Two concrete follow-code defects were reproduced:

1. `MoveSpline::_Finalize` retains the completed spline's `walk` boolean.
   `GetVelocity` used that boolean without checking `Finalized()`. A scripted
   walk followed by client-controlled running could therefore select MOVE_WALK
   (base 2.5 yd/s) while the player ran (base 7 yd/s). Player movement packets
   do not necessarily replace the completed server spline. Existing distance
   catch-up could eventually compensate only after a large gap. This is a
   reproduced code path, not a measurement of the user's particular session.
2. The destination collision call normalized `_range` as an angle instead of
   `_angle`. Changing follow distance therefore changed the direction. Follow
   spline launches also left walk-state inheritance disabled by an unused,
   permanently-false member.

A shared move-type selector now honors only a **live** scripted walking spline
in addition to current movement flags. Velocity, position prediction and walking
selection use the same rule. Every follow launch explicitly sets the correct
walk mode, and collision positioning uses the configured angle. No blanket pet
speed increase was installed. Existing out-of-combat catch-up, combat,
possession/vehicle restrictions, root/casting pauses, map/death and path checks
remain in place. Actual walking, swimming, flight and backward movement retain
their native speed types.

`Test-PetFollowMovement.ps1` compiles extracted production move-type, velocity,
prediction, EnableWalking, complete Creature follow update and PositionOkay
handlers, plus native speed-type selection and actual spline finalization.
It covers the retained-walk reproduction, live/client walking, ordinary run,
boosts, swim/flight/backward movement, catch-up and exclusion gates, explicit
angle/path launch, stale pet walk mode and stop/failure boundaries. Geometry,
network and world objects are recording facades; this is not an in-client
smoothness/pathing certification.

`verify_pet_follow_runtime_release.py` additionally inspects linked Win64
instructions for both the spline done flag and walk field in all three follow
speed/prediction/walk paths, resolving incremental-link thunks. It also checks
the other queue/accessory fixes delivered with the same executable.

Related Scenario LFG lifecycle, Riverbend passenger/data and Warlock spell/mover
regressions pass. Required client confirmation: run straight and around corners,
stop/restart, toggle walking and Burning Rush, then compare follow behavior
before/after entering/leaving a dungeon. Ordinary slight follow distance is
expected; a growing large gap on a clear path remains a failure. If it persists,
capture `GetUnitSpeed("player")` / `GetUnitSpeed("pet")` and the movement state;
the automated reproduction alone does not establish the exact live cause.

Release installed in `build-extractors/bin/Release`, SHA256
`bd37b19788a118c215d03c910947bd471f1e9a66506beb5f161c25ebde36fd9c`.
Original generated game/scripts libraries were rebuilt before final linking;
matching EXE/MAP were copied, with other runtime files preserved. Scoped world
backup and migration 08 are complete. The linked follow/queue/accessory,
Darkheart, ten scenario lifecycle and five handshake call checks pass, as does
`Test-ScepterLogin.ps1 -VerifyReleaseBinary`. Owned hidden validation PID 30112
reached ready at 16:42:34 and shut down gracefully at 16:42:38, exit 0, with no
new Server/DBErrors/stderr errors. No user server was stopped and no validation
server remains. Historical error logs are preserved; client checks above remain
pending. These are automated checks and startup evidence, not client proof.