# Build and runtime convention

- Treat `build-extractors/bin/Release` as the canonical final runtime directory for the compiled `bnetserver.exe` and `worldserver.exe`.
- Temporary and validation builds may use other build directories, but completed Release builds must be built or copied to `build-extractors/bin/Release` before handoff.
- Preserve the runtime configuration files, certificates, DLLs, logs directory, and extracted data directories in `build-extractors/bin/Release`.
- Do not treat `compiles` as the final server launch directory.
