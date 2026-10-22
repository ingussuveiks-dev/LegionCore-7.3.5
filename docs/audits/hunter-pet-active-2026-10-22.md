# Hunter Pet Active and Kill Command, 2026-10-22

Client evidence: Krau's Beast Mastery tutorial is at Pet, Attack! 0/3. Kill Command reports "You can't do that yet", including `/cast Kill Command` with Target Dummy selected. The pet is present and can melee the dummy. Diagnostics: action slots 1..6 are spell IDs 883, 83242, 83243, 83244, 83245, 34026; Kill Command is known, cooldown 0, focus 120; IsUsableSpell is false. A separate screenshot without a selected target reports "There is nothing to attack"; that does not explain the selected-target failure. UI_ERROR_MESSAGE ID 50 alone did not establish an item cooldown; the earlier inference was incorrect.

Read-only local evidence: Krau is specialization 253 and knows active, enabled 137015 and 34026. The permanent Boar is HUNTER_PET (type 1), originally created by taming spell 79598. Native 7.3.5 SpellAuraRestrictions requires caster aura 166615 (Pet Active) for Kill Command 34026. Native spell 166615 retains an area-aura effect. Existing spell_pet_auras bucket -1 casts 166615 on the owner if Beast Mastery aura 137015 is present.

Production CastPetAuras admitted bucket -1 only for six hardcoded creation spells (13481 and Call Pet variants), excluding this saved taming origin. Native pet loading intentionally retains CreatedBySpellId. Two additional lifecycle callers used the old lookup/create flags as the new dispatcher's apply/remove flag: LoadPetFromDB passed `current` although Player::SummonPet calls it with false, and InitPetCreateSpells always passed false.

Repair: use authoritative HUNTER_PET type for shared hunter-pet aura dispatch. For hunter load/create, apply auras for a living pet and remove them for a corpse; preserve other pet types' existing caller behavior. Existing death, resurrection and unsummon cleanup continues to use the dispatcher. Keep row-specific creation/from-spell/specialization predicates. No spell prerequisite, pet-control/range/LOS check, cooldown, scenario criterion, permanent bar layout or character data is bypassed or reset.

Automated checks:

- Test-HunterPetAuras.ps1 compiles the full production CastPetAuras function and native Spell::CheckCast required-aura guard. It reproduces the missing prerequisite, tests taming/Call Pet origins, apply/remove/reapply, current/non-current loads, fresh creation, corpse loading, non-hunter pets, missing specialization, per-row filters and invalid/logging-out owners. Its native C++ DB2 reader validates the real 7.3.5 requirement/effect and the SQL check validates the existing aura row.
- Test-PetLoadSelection.ps1 and Test-WarlockRegression.ps1 passed. This change does not alter tutorial criteria or boost_experience.cpp.
- Release game.lib rebuilt in its original CMake location; only the final worldserver link was staged. The PE checker resolves thunks, verifies the actual pet-type dispatch to bucket -1 and rejects the previous deployed binary. Staged SHA256: `88a4b5cc73f7ffbaaa0833cbfc54b34dbfc21eaf76acf569c8000bc3f4e7d206`.

Client verification remains pending: Kill Command usable with the selected dummy, all three casts credited, later lessons, dismiss/call, death/revive and reconnect. The entire one-opponent/two-opponent/Legion/exit-bird sequence, pet/camera behavior and beach delivery remain unverified. Earlier arrival, ship visibility and NPC foot placement are separately client-confirmed in boost-ship-arrival-2026-10-22.md.

Deployment status before handoff: user-owned worldserver PID 18908 and bnetserver PID 35236 were detected. Neither was stopped. Staging is complete; canonical executable replacement requires the user's worldserver shutdown. No agent-owned server is running.
