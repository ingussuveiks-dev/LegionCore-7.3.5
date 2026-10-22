# Broken Shore taunt victim-selection crash, 2026-10-22

The user reported initially empty-looking beach arrival, NPCs appearing during
loading, then a server crash while attacking/using abilities. The canonical
Crashes directory contains the 15:48:21 text report and minidump. Server.log
records Krau login on map1460 at15:46:53, beach disembarkation15:47:52, and combat
through15:48:21. Both server processes were absent when investigated; no
user-owned server was stopped. Original executable/map, crash artifacts and
Server.log were privately preserved in ignored
.codex/private-backups/taunt-victim-crash-20261022.

## Evidence and cause

The crashing executable SHA256 was
7f8fc186d0cff9803e655ff1f5e16034d3897383511bfaf83f210513fcaf124d.
Matching its actual map resolves the fault to Creature::SelectVictim+0x68,
called by CreatureAI::UpdateVictim, SmartAI::UpdateAI, Creature::Update and the
map update thread. No action-bar handler appears in this stack.

The fault instruction at preferred address0x141326628 is
`mov rcx,qword ptr [rcx]`; the report has RCX=0. The preceding instructions
advance the std::list iterator, read its node's +0x10 AuraEffect pointer, and
dereference that pointer before comparing the iterator to end(). The minidump
contains that node's value at0x2298779e110 and it is zero. The list-head field
itself was not captured; this is not a full heap reconstruction or identification
of the particular NPC/taunt spell. The instruction sequence and source match
the unsafe do/while in Unit.cpp's Creature::SelectVictim.

When the newest taunt's caster is absent or dead, the old nonempty-list branch
unconditionally increments then dereferences the iterator. One stale taunt
therefore reads end(); multiple unavailable taunts eventually do the same. An
older dead caster could also be selected without an explicit alive check. The
latest local TrinityCore uses a different threat-manager architecture and is not
an interchangeable implementation for this Legion core.

## Repair and checks

Replace the do/while with a bounded for loop: advance, compare with end(), then
read the effect. Older candidates must be alive, visible, valid attack targets
and accessible. When none qualifies, retain existing normal threat/helper/evade
selection. The newest live taunt behavior and unrelated threat, passive, pet,
owner/guardian, casting, vehicle and nearest-target branches remain unchanged.
This fixes the shared core path without faction/NPC/spell whitelists or disabling
taunts, attacks or diagnostics. The precise user action that made the caster
unavailable is not established.

- Test-CreatureTauntVictim.ps1 -BaselineRevision 16737b5a reproduces the old
  end-sentinel dereference deterministically with a single absent pet taunter.
  The fixture uses actual std::list storage with checked iterators and extracts
  the complete production SelectVictim body rather than a replacement algorithm.
- The fixed body passes absent/dead single and multiple taunts, every older
  position, surviving player/pet casters, invisible/invalid/inaccessible/dead
  candidates, empty taunts, normal threat, passive/helper/owner/guardian,
  casting, vehicle, nearest-target and evade paths.
- The linked PE check confirms the actual selector checks the sentinel before
  aura dereference, branches past it at end(), uses native Aura::GetCaster and
  rejects dead older casters. It rejects the archived crashing executable and
  passes the deployed executable, resolving incremental-link thunks.
- Test-HunterPetAuras.ps1 -VerifyReleaseBinary, Test-WarlockRegression.ps1 and
  Test-ScepterLogin.ps1 -VerifyReleaseBinary pass. These are automated checks;
  they do not replay the user's full Broken Shore combat session.

Release game.lib was rebuilt in its original CMake location and worldserver
linked into runtime-test/creature-taunt-victim/Release. Executable/map were copied
to build-extractors/bin/Release with matching SHA256
e7694ac0e8d1145f34a455b4380e7be6d8b836cf9a7a1159954eaa4309b17cb4.
Configs, data, DLLs, certificates and character state were not replaced.
Owned hidden validation PID17480 reached ready without ERROR/database-loader
errors and shut down gracefully with exit0; it was not left running.

## Remaining client evidence

Repeat beach arrival and combat with Krau, including pet Growl and loss/death of a
taunter during an active taunt. Normal NPC combat and progression should continue
without a crash. This exact client replay remains pending. Initially delayed NPC
appearance is not proved fixed by this iterator repair. Earlier login diagnostics
about a stale non-permanent tutorial bind and an unexpected suspend-token response
are separate observations; startup alone does not exercise or resolve them.
