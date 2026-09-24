# Build and runtime convention

- Treat `build-extractors/bin/Release` as the canonical final runtime directory for the compiled `bnetserver.exe` and `worldserver.exe`.
- Temporary and validation builds may use other build directories, but completed Release builds must be built or copied to `build-extractors/bin/Release` before handoff.
- Preserve the runtime configuration files, certificates, DLLs, logs directory, and extracted data directories in `build-extractors/bin/Release`.
- Do not treat `compiles` as the final server launch directory.

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
