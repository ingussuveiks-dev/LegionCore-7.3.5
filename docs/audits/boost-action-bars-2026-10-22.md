# Specialization starter action bars, 2026-10-22

The user reported Krau's Beast Mastery attacks scattered across side bars,
behind generic UI/book buttons, after completing the boost ship tutorial. They
requested a personal repair and automatic specialization layouts for everyone
finishing that tutorial. Both user servers and the client were shut down before
maintenance; no user-owned process was stopped by the agent.

Cause: PopulateBoostActionBar sorted spells by learning level/ID and filled vacant
slots without displacing the original generic buttons. The departure loop then
filled more vacancies in numeric spell order. The common ClassMask=0 skill-row
admission also accepted profession-book openers and generic UI spells which are
not profession skill-grant spells according to IsProfession().

BoostActionBars.h supplies CUSTOM PvE starter priorities for all 36 Legion
specializations. These are ergonomic button layouts, not a fixed cast sequence
or a claim of optimized retail DPS. The actual priority still depends on talents,
resources, procs, targets and encounters. Only currently active, known, visible
abilities are admitted. Unlearned artifact/talent powers are omitted; native
GetCastSpellInfo replacement resolution prevents e.g. Dire Beast/Dire Frenzy
duplication. Class abilities/racials form the fallback, while profession books,
generic UI buttons, mounts, hidden and passive spells are omitted. Curated
current-spec actives can be admitted without a SkillLineAbility row.

Visible ordinary slots are assigned in page order 0,60,48,24,36,12. Existing
nonspell buttons (including 64-bit macros, items and flyouts) are retained after
the abilities. The entire plan is checked for overflow before modifying any
actions. Existing stance pages remain intact. Empty druid Cat/prowling/Bear/
Moonkin and rogue Stealth/Shadow Dance pages receive appropriate known attacks,
so changing form does not expose an empty primary bar. Native
SpellShapeshiftForm.db2 confirms Cat/Stealth bonus1, Dance2, Bear3, Moonkin4.
Actual form paging remains a separate client check.

The same arranger runs during boost initialization/finalization and once in the
departure bird's phase2, after all lessons and before the leave spell. The
tutorial packet overlay stays intact until OnMapChanged restores the normal
bar. Ordinary login/map changes/spec switches do not reapply these layouts.
Native action dirty states save normally on autosave/logout; no new airborne
character-position checkpoint is forced.

Krau was confirmed offline on map1460 with specialization253/active spec0.
His previous action rows and executable/map were privately backed up under
ignored .codex/private-backups/krau-action-bars-20261022. A native DB2 reader used
the shared production planner, saved active spell rows plus previously accepted
default actions, class/spec/talent admission and replacement data to generate a
reviewed, offline-guarded SQL transaction. Only his active-spec ordinary buttons
were repaired. Other characters/specs, known spells, talents, inventory and
quest/scenario position were not edited. No personal rows/backups or credentials
are committed. Main slots 1..12 now contain Kill Command, Dire Frenzy, Cobra Shot,
A Murder of Crows, Multi-Shot, Bestial Wrath, Aspect of the Wild, Binding Shot,
Counter Shot, Exhilaration, Aspect of the Turtle and Disengage.

Automated checks passed:

- Test-BoostActionBars.ps1 executes the extracted production arranger and shared
  planner: known/active gates, replacements, talent/spec/skill admission, passive/
  hidden/profession/generic exclusion, racial inclusion, duplicates, preservation
  of macros/items/stance pages, empty form initialization, atomic overflow,
  unknown-spec failure and all36 profiles. Its native catalog verifies every
  profile/utility spell ID in the runtime 7.3.5 Spell.db2 and form bonus indices.
- Test-ActionBarPersistence.ps1 -VerifyReleaseBinary covers the unchanged native
  add/remove/save/load states and specialization isolation.
- Test-BoostConstructLessons.ps1 and Test-BoostShipArrival.ps1 pass. The linked
  construct/arrival checks and Test-ScepterLogin.ps1 -VerifyReleaseBinary pass.
- The new PE check resolves actual boost, finalization and departure calls into
  the arranger, shared planner and native action mutation/send functions.

Release game.lib and scripts.lib were built in the original CMake locations;
only the final executable was linked to staging. Executable/map were copied to
canonical build-extractors/bin/Release, preserving configs/data/DLLs/certificates.
Deployed SHA256: 7f8fc186d0cff9803e655ff1f5e16034d3897383511bfaf83f210513fcaf124d.

The canonical executable reached ready in an owned hidden validation process
(PID25812), with no ERROR or database-loader errors, and shut down gracefully
with exit0. A final process check found no worldserver/bnetserver processes.
Krau's repaired main twelve SQL rows were read back successfully while offline.

Client status: the earlier deployed build's Horde BM ship traversal was confirmed
by the user before this change (reached the island). The new ordinary bar layout,
its logout/restart persistence, actual form pages, Alliance and other specs still
require client checks. Actor/scenario progression was not changed by this fix;
the previous successful traversal does not itself verify the new departure bar.
