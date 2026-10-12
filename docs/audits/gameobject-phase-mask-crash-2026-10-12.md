# Gameobject phase mask startup crash

The previously inspected `compiles/Crashes/_worldserver.exe_[20-9_20-48-10].txt` report is from **2026-09-20 20:48**, revision `3febaa3fed09+`, RelWithDebInfo. Its named stack is `Trinity::Assert -> BaseDatabaseResultValueConverter::LogTruncation -> GetUInt16 -> ObjectMgr::LoadGameobjects`, at `ObjectMgr.cpp:2675`. The access violation in this report is the fatal assertion path after a narrowing conversion.

The current world database defines `gameobject.phaseMask` as `INT UNSIGNED`, and `GameObjectData::phaseMask`, `WorldObject::SetPhaseMask` and the gameobject save path use `uint32`. The loader alone requested `GetUInt16`. Changed it to `GetUInt32`, preserving every valid bit through `0xFFFFFFFF` without changing the global database conversion checks. Existing zero-mask normalization remains in place. No SQL migration or spawn-mask changes are required.

Before deployment the current maximum gameobject mask was 65535, with no rows above that value; this explains why recent startup succeeded despite the latent bug. Existing rows were not changed or artificially enlarged for testing. The separate corpse reader uses `GetUInt16` against an actual `SMALLINT UNSIGNED` column; it is not the reported mismatched read and is outside this fix.

`Test-GameObjectPhaseMask.ps1` compiles the actual loader assignment with the production numeric conversion templates. It checks text and binary values for 0, 1, 65535, 65536, 65537, `0x80000000`, `0x80000001` and `0xFFFFFFFF`. The test replaces only the fatal truncation reporter with an exception and confirms the old 16-bit getter rejects the high values while the repaired read preserves them. No live database writes are made by this regression test.

Release build and handoff use `build-extractors/bin/Release`; this fix does not delete crash reports. This fix does not change the level-100 boost, ship scenario, Aviash flight, vehicle or pet code.
