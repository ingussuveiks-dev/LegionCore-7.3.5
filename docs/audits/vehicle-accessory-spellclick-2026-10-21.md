# NPC accessory boarding versus player spellclick actions

DBErrors.log contained repeated "not a valid vehicle enter aura" records for
230986 and 186608 during the user's earlier world traversal. The native data
and live world rows distinguish these from actual boarding spells:

| NPC | Player click | Native effect | Accessory data |
| --- | --- | --- | --- |
| 114970 Deline's Gondola | 230986 Get In | force-casts 228951, which summons gondola 114971; no direct CONTROL_VEHICLE aura | seven NPCs in seats 1–7 |
| 97648 Grasp of Dargrul | 186608 Sundering Stone | rescue dummy/periodic trigger; no CONTROL_VEHICLE aura | obsolete generic captive 95051, seat 0 |

`Vehicle::InstallAccessory` called `HandleSpellClick(accessory, seat)` for an
internal NPC passenger installation. That path requires a direct control aura
for an explicit seat and rejected the unrelated player action, producing the
errors and leaving accessories unboarded. A successful startup did not exercise
this: the errors occurred when their world grids loaded.

Internal accessories now use the native `Unit::EnterVehicle` path. It casts
46598 CONTROL_VEHICLE (native aura 236) with base point seat+1. Owner, minion,
summon lifetime/type and transport offsets are retained. The existing recorded
force-aura branch keeps its seat -1 behavior. Actual player spellclick rows,
requirements and handlers are unchanged, and accessory installation no longer
dispatches player-click scripts as a side effect. Native passenger boarding
callbacks continue through the ordinary ride aura.

Riverbend's current `npc_grasp_of_underking_quest` already selects, summons and
tracks the correct captive for each Grasp spawn. Enabling the old generic
95051 accessory would occupy its seat with an untracked/wrong captive and break
the script's GUID check. Migration 08 therefore removes only that obsolete row,
conditional on the current quest script binding. Deline's seven rows and both
player-click spells are retained. The Riverbend script itself is unchanged.

`Test-VehicleAccessoryBoarding.ps1` executes extracted production installation
and actual Unit::EnterVehicle. It checks seven explicit seats/seat+1 values,
the force-aura branch, owner/minion/offset/lifetime, uninstall/no-map/no-summon
failures and absence of player-click dispatch. Shadow SQL recreates the broken
Grasp row and repeats the scoped migration while preserving the seven Gondola
rows and both click actions. The production native DB2 reader confirms spell
46598's control aura and the two non-boarding click spell effects. It also loads
the actual Vehicle/VehicleSeat tables and verifies all seven Gondola seat slots
on vehicle 5016 and Grasp seat zero on vehicle 4397, rather than trusting only
SQL seat indices. The live templates are checked against those vehicle IDs.

Existing Riverbend release/wrong-seat/empty-seat/retry/group credit and world
data tests pass. Warlock scripted mover and spell regressions pass. A linked PE
check confirms InstallAccessory calls EnterVehicle and does not call
HandleSpellClick. Full client checks remain: load Deline's Gondola and verify
NPC passengers/normal Get In; revisit all three Grasp variants and complete
their rescue interaction; inspect new DBErrors records. No old logs were erased.

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
