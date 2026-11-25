# Faronaar personal companion accumulation

The player's screenshot at Legion Camp: Ruin showed a stack of mounted Illidari. Local creature data matches the personal 90474 template and its mount 67597. Azsuna has no placed copy of 90474, and the matching 26972 effect for 178860 requests one summon.

The replacement path schedules each new follower's five-minute limit with `DespawnOrUnsummon(300000)`. That sets the despawn flag immediately. Later `DespawnOrUnsummon()` calls from follower recovery and teardown return at that flag, leaving the old follower alive. Recovery can repeatedly select that same out-of-range follower and add new ones until its timer expires.

Recovery and teardown now call the native immediate `TempSummon::UnSummon()` on the actual player's companion. The lookup skips deletion-queued actors so a replacement can be identified even before the removal list is drained. The native summon spell, five-minute fallback, combat and travel deferral, and six-second loss detection remain in place. This change does not award quest credit.

Release rebuilt `faronaar_chain.cpp` in `scripts/Release/scripts.lib` and linked a separate non-incremental worldserver using the original Release game and scripts libraries. Its map records the companion cleanup and the prepared multi-quest dialog. Staged executable SHA-256 `44BD64B6FA67B17FD98005D1A604EB2154F79BEC1C4AA31330DBFE9E9B624843`. DBErrors was empty and no crash reports were present during inspection. No separate test server or automated test was run during the active gameplay report. Deployment and client recheck of follower separation and cleanup are pending.
