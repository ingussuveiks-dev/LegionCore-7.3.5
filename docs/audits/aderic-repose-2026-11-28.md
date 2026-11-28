# Aderic's Repose and Keel Harbor

`Laid to Rest` (24602) requires five Unearthed Mementos (49921). The shared Disturbed Soil (201871) used native Lock 43 and contained one guaranteed memento, but its loot row had `QuestRequired=0` and the chest had no quest gate. A player without the quest could consume shared stock. The alternative Disturbed Spirit (37884) row had `LootMode=0`, so the ordinary loot roll never considered it.

The repair uses the existing quest chest guard for this object on map 654. The marker, use and loot paths check the active quest and remaining need. The migration sets the original chest and spirit mementos to quest loot and enables ordinary loot mode for the spirit. The spirit's original 0.2036% chance, the shared chest respawn and the placed objects are retained. Creature loot still uses the existing owner and eligible-group dispatch. The test checks the real spirit table and recipient eligibility, with no random spirit kill or pet final blow asserted.

## Verification

- `Test-QuestObjectLootNative.ps1 -AdericRepose` used an isolated database, the actual Gilneas spawns and the 26972 DB2 data. The ordinary lock spell and server loot packets delivered one memento each at five sites. Shared stock remained available when a player without the quest tried the first site or a player with a full objective tried the sixth. Rewarding 24602 removed the mementos and unlocked 24679.
- Krennan gave the original Blessed Offerings (51956). Spell 72853 rejected use away from focus 1643. Abandoning and accepting again replaced the item. At Aderic's Tomb the actual cast spawned 202319 and completed only the caster's objective. The normal reward path removed the source item and applied aura 70695 and phase 189. Darius became available at Keel Harbor for the eligible player. His real 24680 reward opened 24681.
- The four existing `-SupplyChests` routes passed through the same core code, covering 90 placements. The final native tests exited cleanly with no appended DB errors, server errors or crash reports.
- Release linked the final game and scripts libraries from the original CMake paths. The promoted `worldserver.exe` SHA-256 is `D1CA340F248BA9D5994202E535B385F7F8F28E09F7538E576ECBE3F5ECC3EFFF`. The canonical updater recorded `2026_11_28_04_aderic_repose.sql` as `RELEASED`, and live data matched all three edits. The owned startup reached ready and shut down with exit 0. Its crash and appended-log comparison was clean.

## Remaining client and story checks

Actual client looting, pet final blows, the burial presentation and the cemetery's post-reward visibility remain open. The 70.55-second reference footage was sampled every five seconds from offer through return and shows a burial cutscene. Neither the item spell's two native effects nor the local quest rows identify its presentation trigger. The 26972 scene catalog lookup did not find a matching Gilneas package by name. The recording is older than Legion and supplies visual sequence evidence only. [Patriarch's Blessing gameplay](https://www.youtube.com/watch?v=qXSdQxB_Ec0)

## Local diagnostic archive

The ignored `build-extractors/bin/Release/fixed/2026-11-28-aderic-repose/` contains the hash-verified diagnostic log, promotion record, one rollback copy and cleanup manifest. An early failed fixture assertion unwound a still-published tomb and caused an access violation, followed by eight aborted private MariaDB connections. The fixture now removes the tomb during destruction, and both final native tests closed normally. No active crash file was created. The existing mixed 1,347-byte canonical DB error log was retained. The cleanup removed 27 generated files totaling 905,336,309 bytes.
