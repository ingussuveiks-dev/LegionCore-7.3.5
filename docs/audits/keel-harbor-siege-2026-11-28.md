# Keel Harbor siege and the Endgame offer

Lord Darius Crowley starts and ends `They Have Allies, But So Do We` (24681). The post-siege quest `Endgame` (26706) starts at Lorna Crowley (43727, spawn 148161). Lorna and Darius are present in the same Keel Harbor phase before the siege, and 26706 had `PrevQuestID=0`. The old data allowed the ordinary quest-accept packet to start Endgame before the player even accepted 24681. The migration sets 26706's predecessor to rewarded 24681. The original giver, receiver, counters and phase spell 74093 are retained.

## Server-side verification

- `Test-QuestObjectLootNative.ps1 -KeelHarbor -KeelHarborObserve` reproduced actual premature acceptance at Lorna with the old data. The same probe with the migration rejected the early offer, including while the three counters were complete and the reward was pending.
- Original rack 38150 at spawn 148959 used spell 71238 and properties 488 to summon Glaive Thrower 37927. The original ride spell 68503 put the user in seat 0 of vehicle 866. An unqualified player's click created nothing. A repeat click preserved the active vehicle, and the ordinary exit packet restored the player as mover. The player could then board again.
- The verified 26972 vehicle still offered Double Speed (59737), Glaive Barrage (70732) and Launch Glaive (70735). Both attacks hit the three original battlefield creature types. Native spells and death handling awarded 40 Orc Raiders (37916), 8 Wolfmaw Outriders (37938) and 4 Orcish War Machines (37921) to the controller. The fixture reused placed target locations and stepped spell events between shots. Client aiming, terrain travel, Double Speed timing, party and separate combat-pet paths remain open.
- Darius's normal turn-in activated aura 74093 and phase 191 for the owner. An ordinary acceptance at Lorna then opened Endgame. The preceding `-AdericRepose` native check also passed through the Keel Harbor turn-in.
- The canonical startup applied `2026_11_28_05_keel_harbor_siege.sql` and recorded `RELEASED`. A read-back confirmed `26706 -> 24681`. The owned server reached ready and exited cleanly. Both canonical executable hashes matched the preceding release. The final native and canonical runs recorded no new crashes, unexplained warnings or appended DB errors.

The 510.276-second reference recording was sampled every eight seconds from boarding through manual return and reward. It shows Glaive Thrower combat and a second boarding after the first vehicle run. It predates Legion. The matching 26972 spell, faction, stage and seat data supplied the server inputs. [Siege gameplay](https://www.youtube.com/watch?v=UxYRU0lw-RM)

## Diagnostics and next quest

The isolated probe initially lacked normal reputation login initialization and spell bonus loading. Those omissions produced apparent zero-damage shots and one fixture-only access violation. The trace identified the missing bonus store, and the fixture now initializes both through the normal loaders. MariaDB reported eight aborted fixture connections at that instant and later shut down normally. The final native tests closed normally. Hash-verified diagnostics and the deletion manifest are in the ignored `build-extractors/bin/Release/fixed/2026-11-28-keel-harbor-siege/`. Cleanup removed 35 generated files totaling 1,127,794,690 bytes. The pre-existing mixed 1,347-byte DB error log remains available.

The Endgame flight itself remains open. The 26972 data already link the dock Hippogryph's spell 81877 to 43751 and identify 81779 as the personal finale credit 43729. The summoned 43751 currently has vehicle ID zero. These data observations need to be checked through actual boarding, flight and gunship completion before changing the route.
