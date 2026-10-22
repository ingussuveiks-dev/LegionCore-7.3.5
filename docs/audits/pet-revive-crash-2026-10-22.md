# Missing dead Hunter pet revival crash, 2026-10-22

The user reported that their pet had disappeared and Revive Pet crashed the
server. The canonical Crashes directory contains the16:11:24 text report and
minidump. Server.log records spell982 being placed on a button at16:11:22. A
read-only character-pet query found the existing Boar in active slot0 with saved
health0. This supports a dead/missing pet rather than only a hidden portrait.
No character/pet database values were reset or manually revived.

The crashing executable SHA256 was
e7694ac0e8d1145f34a455b4380e7be6d8b836cf9a7a1159954eaa4309b17cb4.
Its matching map resolves the fault to Pet::setDeathState+0x90, called by
Pet::LoadPetFromDB, Player::SummonPet and Spell::EffectResurrectPet. The preferred
fault address0x1418b6530 is `mov dword ptr [rax+50h],ebx`, with RAX=0. That writes
PetInfo::Health after dereferencing GetCurrentPet() without a current selection.
The original executable/map, report/dump, Server.log and pet-state snapshot are
preserved privately in ignored .codex/private-backups/pet-revive-crash-20261022.

## Lifecycle and repair

LoadPetFromDB reconstructs the saved corpse before assigning CurrentPetIndex and
SetMinion. This is legitimate loading order: the selected PetInfo exists, while
GetCurrentPet() may still return null. The corpse-state hook incorrectly assumed
it was already current; if a different current selection existed, it could also
write that other pet's health. The newer local TrinityCore uses similar loading
order but has no current-record write in this corpse hook.

PetStable::UpdatePetHealth now finds the exact persistent PetNumber in active or
unslotted records. The death hook obtains that number from CharmInfo and updates
the matching health without changing the selected index. It safely handles an
unrecorded fresh pet, missing owner/stable/charm and zero number, while leaving
stabled pets and unrelated current records unchanged. Existing corpse flags,
death aura removal, revival aura reapplication, charm cleanup and native revive
health/save/follow-command logic remain in place. The revive effect also stops
before constructing a slot if there is no stable or no dead active candidate.

## Verification and deployment

- Test-PetReviveLifecycle.ps1 -BaselineRevision 72b05421 reproduces the original
  ACCESS_VIOLATION. Only this baseline test handles the exception with Windows
  SEH; production does not catch or suppress it. The test uses native PetStable
  records and extracts the real selection, synchronous dead-load/slot binding,
  full death hook and full Revive Pet effect, preserving their original order.
- Fixed-handler tests pass for all five active slots with no selected pet,
  another current pet, repeated death/revive, restored health, pet aura calls,
  stay/attack/follow/return resets, corpse reposition, unslotted/fresh/summoned
  pets, no dead candidate, missing dependencies and unrelated spell modes.
  Creature base transitions, aura delivery and DB writes are fixture boundaries;
  real packet animation/portrait and asynchronous spell loading need the client.
- The PE check resolves chained unwind ranges and confirms the actual death
  handler uses persistent identity, compares active/unslotted PetNumber before
  health stores, and no longer calls GetCurrentPet. It also confirms both revive
  admission guards. The archived crashing executable fails this check.
- Test-PetLoadSelection.ps1, Test-HunterPetAuras.ps1 -VerifyReleaseBinary,
  Test-WarlockRegression.ps1 and Test-ScepterLogin.ps1 -VerifyReleaseBinary pass.
  The previous taunt victim-selection guard also passes its deployed PE check.

Release game.lib was rebuilt in its original generated location and worldserver
linked into runtime-test/pet-revive-lifecycle/Release. Executable/map were copied
to canonical build-extractors/bin/Release with matching SHA256
4651b7b349c543f852ee5dfa1c871d8a215da75f8a7ad3a1cf7b6f945d926518.
No user server was running or stopped by the agent. Owned hidden validation
PID13812 reached ready with no ERROR/database-loader errors, then shut down
gracefully with exit0. Configuration, DLLs, certificates and extracted data were
preserved. No hidden server remains.

Client retest remains pending: log in with Krau and revive the existing Boar;
confirm the pet and portrait appear, Kill Command works, and death/revive plus
logout/reconnect preserve the same pet. The exact earlier cause of the pet's
death and corpse disappearance was not established from this crash stack.
