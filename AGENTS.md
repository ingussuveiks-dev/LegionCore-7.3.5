# Build and runtime convention

- Treat `build-extractors/bin/Release` as the canonical final runtime directory for the compiled `bnetserver.exe` and `worldserver.exe`.
- Temporary and validation builds may use other build directories, but completed Release builds must be built or copied to `build-extractors/bin/Release` before handoff.
- Preserve the runtime configuration files, certificates, DLLs, logs directory, and extracted data directories in `build-extractors/bin/Release`.
- Do not treat `compiles` as the final server launch directory.

# Legion 7.3.5 shop previews

- For Mounts and Pets store offers, set `battlepay_display_info_visuals.DisplayId` to the collectible's `CreatureDisplayInfoID` and `VisualId` to the Store UI model scene `10`. Set `battlepay_display_info.FileDataID` to `10` as well.
- Do not copy `Mount.UiModelSceneID` or `BattlePetSpecies.CardUIModelSceneID` (`4`/`6` for many entries) into these Store scene fields. Those values caused empty cards and previews even when the model ID was correct.
- Check both the catalog card and expanded preview in the 7.3.5 client after adding offers. Luminous Starseeker is a known working scene-10 reference; the blank Shackled Ur'zul and Shadow cards exposed this regression.
