# Build and runtime convention

- Treat `build-extractors/bin/Release` as the canonical final runtime directory for the compiled `bnetserver.exe` and `worldserver.exe`.
- Temporary and validation builds may use other build directories, but completed Release builds must be built or copied to `build-extractors/bin/Release` before handoff.
- Preserve the runtime configuration files, certificates, DLLs, logs directory, and extracted data directories in `build-extractors/bin/Release`.
- Do not treat `compiles` as the final server launch directory.

# Commit and push convention

- After completing a fix and its relevant checks, create a focused commit with the implementation, regression tests and audit, then push the current branch to `origin`. This is the user's standing preference; report the commit hash and distinguish automated checks from pending client verification.
- Keep separate problems in separate commits. Review the staged diff, exclude runtime data, credentials and personal character backups, and do not force-push shared history.

# Legion 7.3.5 shop previews

- For Mounts and Pets store offers, set `battlepay_display_info_visuals.DisplayId` to the collectible's `CreatureDisplayInfoID` and `VisualId` to the Store UI model scene `10`. Set `battlepay_display_info.FileDataID` to `10` as well.
- Do not copy `Mount.UiModelSceneID` or `BattlePetSpecies.CardUIModelSceneID` (`4`/`6` for many entries) into these Store scene fields. Those values caused empty cards and previews even when the model ID was correct.
- Check both the catalog card and expanded preview in the 7.3.5 client after adding offers. Luminous Starseeker is a known working scene-10 reference; the blank Shackled Ur'zul and Shadow cards exposed this regression.

# Level 100 boost tutorial regression

- After changing `boost_experience.cpp` or scenario criteria handling, check the full 7.3.5 sequence: one opponent surrenders and gives 1/1, then two separate opponents surrender and give 2/2, then Legion attackers spawn, then the exit bird boards and delivers the player. Do not advance a wave from spawning or from an unrelated death.
- Include a spell that can kill a fresh sparring opponent in one hit. The instance damage hook must turn that hit into a surrender before damage is applied, even when a transport passenger's AI is unavailable. Check the `CreatureDies` fallback log; it should not occur in a normal run.
- Verify the matching `ScenarioStep.db2`, `CriteriaTree.db2`, and `Criteria.db2` entries from the 7.3.5 runtime data, compile the Release server, and test in game before calling the sequence verified.
- Surrender must compare final damage AFTER `GetHealthMultiplierForTarget` with `GetHealth(attacker)` / `GetMaxHealth(attacker)`. Comparing raw damage to template health allowed scaled one-shot kills to bypass surrender.
- Preserve the 2-second pause before the two-opponent wave. The 12 Legion attackers should initially divide evenly across the player and five deck allies, including the trainer; ally kills must count for the scenario.
- Reuse the clicked exit bird and wait for completed vehicle boarding before takeoff. Broken Shore disembarkation must also work without client scene callbacks and must detach the player before placing them beside the beach allies.
- A `creature_template_scaling` row with a `0..0` level range is not active level scaling. Check both template and target-relative health at spawn; the 2026-09-24 test had 2 target-relative HP and billion-point normalized hits, causing immediate surrender. Keep the invalid-range regression test.
- Verify the camera follows the departure bird in the client, not just that the server reports a passenger. Restore the player's viewpoint on dismount and before transferring maps.
- A farsight-only departure failed the 2026-09-24 client retest. Preserve the scripted flight's `SetClientControl(bird, false)` active-mover handoff after boarding and before starting the spline. Restore `SetClientControl(player, true)` on dismount, map transfer, and fallback; this non-control seat does not run generic charm cleanup.
- Check the permanent pet and its portrait after boarding, dismounting, changing maps, summoning a secondary guardian, and resummoning the same demon. Unslotted warlock pets must reload by `PetNumber` when requested by number, and secondary guardians must not overwrite `UNIT_FIELD_SUMMON` for an existing primary pet. Run `tools/tests/Test-PetLoadSelection.ps1` when changing pet lookup.
- If the pet exists but its portrait disappears after the departure flight, check `UnitExists("pet")`, `PetUsesPetFrame()`, `UnitInVehicle("player")`, `PlayerFrame.vehicleHidesPet`, and `PetFrame:IsShown()`. The 2026-09-24 client reported `true true false true false`: the Lua vehicle flag stayed set despite leaving the vehicle. The 7.3.5 PlayerFrame clears that flag on `UNIT_EXITING_VEHICLE`. Preserve the exit spline before the Broken Shore queue teleports the player, and let all 219912 passenger-targeted effects finish before removing passenger 0. Confirm the flag becomes false and the portrait returns in the client; a successful build or pet GUID check does not verify this.

# Warlock spell regressions

- Burning Rush (111400) must toggle off on recast. Its low-health periodic hook must call `PreventDefaultAction()` before removing the aura; removal alone does not stop the current damage tick.
- Shadow Shield (115232) effect 0 reduces damage; effect 1 triggers 115234, whose script consumes a stack. Do not trigger a second retaliation from effect 0 or look up spell ID 0.
- Run `tools/tests/Test-WarlockRegression.ps1` after changes to these paths or scripted mover resynchronization. The tests compile extracted production handlers; client confirmation of toggling, visuals, pet portrait, and camera remains necessary.
- Ordinary Enslave Demon sets a charmer without giving the NPC an owner. `CharmInfo::InitCharmCreateSpells` must handle a null `GetOwner()` when checking high-level instance spell restrictions. The 2026-10-14 crash was this dereference after leaving the New Blood cage; run `tools/tests/Test-CharmCreateSpells.ps1` for ownerless, owned, guardian and map/level branches.

# Scripted quest and visual regressions

- The New Blood (40729, scenario 962) exposed a gap between checking SQL rows and checking an executable interaction. For each stage verify initial spawn/pose, client-visible interaction flags, cast target validation, action completion, credit exactly once, cleanup, and the next stage. Include abandonment/retry, re-entry, pet killing blows, both factions, and later scenarios reusing the same NPC entries.
- Source-18 spellclick conditions can hide a client's NPC flag. A guard death or scenario-step change does not automatically dirty that field. Verify its actual update and then the spell's explicit target checks; protected friendly NPCs can still reject assistance. Run `tools/tests/Test-NewBloodPrisoners.ps1` for the prisoner lifecycle and target regression.
- Trace a visual through native SpellEffect/SpellXSpellVisual, visual kits, attachment points, model and animation data. A matching spell name is not visual evidence: 199994 uses Lightning_Fel_Cast_Hand, while portal model 607277's fire is in CustomSpell02/AnimKit 1244. Preserve the portal's 13-second Birth animation; do not replace it with a fully assembled falling summon.
- Rescue prisoners 98775/98783/98793 are already elevated in their map-1522 DB positions. Native chain spells are 192195/192196/192198/192200, pose 192214, release 192495. Keep helpers as aura casters, remove all chains/pose on release, and wait for landing before running the existing credit/follower lists. Archive summons must not receive the prison lifecycle.
- Never describe a whole quest or visual as client-verified based only on compilation, SQL presence or simulated tests. Record evidence, assumptions, missing client checks and observed failures in the quest audit; retain them across subsequent fixes.
- In The New Blood, wounded Ritssyn intentionally stays behind after the cage escape; Calydus guides the finale. Preserve the archive summoner GUID when handing off to that guide. SmartAI StartPath refuses actors in combat, so test the combat prerequisite and failed/missing-guide retries before consuming the transition.
- A failed SMART_ACTION_CAST can return before its linked action runs. Essential progression and independent visuals must not depend on an unrelated conversation cast succeeding. Test failure paths as well as SQL row presence; confirm native Birth/fire/hand visual playback separately in the client.
- The archive regression must begin with no Jagganoth: actual tome-stage completion must invoke Calydus's summon, then run Jagganoth's dialogue/smash and the exact guide's escape handoff. Starting a test with the boss already spawned missed the 2026-10-14 failure. Persistent scenario progress must not depend solely on a transient SmartAI phase set by an earlier dialogue chain.
- Re-entry tests must include scenario construction before the final login access check. Scenario's constructor consumes `Player::GetScenarioId()`; validate the live instance's scenario ID after `AddPlayerToMap`, and restore the selector before `CreateMap` chooses difficulty. The old test checked access before consumption and missed the Orgrimmar fallback. A fresh New Blood instance restarts at the prison because actor/criteria state is not persisted; do not claim exact-stage recovery after a crash.
