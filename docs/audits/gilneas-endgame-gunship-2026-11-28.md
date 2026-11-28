# Endgame gunship and dock interaction

The matching 26972 transport template for the Orc Gunship (203428) retained taxi path 2338, display 8253 and passenger map 749. The `transports` startup table had no row for 203428. The dock Hippogryph (43747) also offered its existing 81877 interaction without checking for active Endgame (26706).

The repair adds one continent transport and uses the existing Gilneas phase 191 and server mask 4194304, granted by aura 74093 after quest 24681. The ship phase is set before collision model creation and world publication. Immediate transport publication checks the same phase admission as login and visibility refresh. The original 81877 interaction now requires 26706 in progress.

## Verification

- The native observation with pre-migration data found the missing continent registration and the unguarded dock interaction. The repaired probe used the actual spawn, quest acceptance packet and spell-click condition loader. An unrelated player could not start a summon. Accepting the quest admitted the interaction, abandonment closed it and reacceptance restored it.
- The actual transport reached its recorded stop on path 2338. The native frame keeps a 180-second pause from arrival event 25670 to departure event 25664. The probe saw the ship leave, found native deck collision for the eligible player and checked that the same deck did not provide collision outside that phase. These checks awarded no finale objective.
- Production linking used the original CMake game and scripts library paths. The deployed `worldserver.exe` SHA-256 is `5FD82540768E75FD3409D8E7FFB044147764824334C42C0AAD849E94DE2F7010`. The canonical updater recorded `2026_11_28_06_gilneas_endgame_gunship.sql` as `RELEASED`. Live data confirmed one transport and one matching dock gate. The owned startup reached ready and exited 0. Final test diagnostics reported zero new crashes, server warnings or appended DB errors.

The 360.85-second 2016 Legion recording was sampled every seven seconds from the dock through the flight, deck traversal, the lower gunship encounter and the wyvern return to Lorna. The archived ArkCORE SQL includes the 4375101 flight, 4371301 escape and transport-relative passenger coordinates. Its older implementation is a reference and its data still require checks against the 26972 model and local APIs before they can drive progression. [Legion gameplay](https://www.youtube.com/watch?v=-XLIPxyjd4g) · [Archived outbound route and crew](https://github.com/Arkania/ArkCORE-NG/blob/a7304c3075bf8ee7eb5bdf45f31cda01c8626b52/sql/old/Old%202016/_world_2016_07.zip) · [Archived follow-up route and crew updates](https://github.com/Arkania/ArkCORE-NG/blob/a7304c3075bf8ee7eb5bdf45f31cda01c8626b52/sql/old/Old%202017/_world_2017_05.zip)

## Open progression

The level-quest audit remains open for 26706. The verified dock gate admits the real native cast. The 43751 and 43713 templates still have vehicle ID zero, and passenger map 749 is empty. Tobias's signal, real boarding, the deck and engine room sequence, source 81779 completion and the return to Lorna need connected verification. The original finale cast remains the objective credit source.

## Local diagnostics

An early test fixture omitted the page-text loader and produced gameobject validation errors. It now runs that loader before loading templates. Both final native runs and the private database shutdown were clean. The two existing MSVC enum arithmetic warnings in the transport spline calculation still appear during compilation. Hash-verified evidence, extracted legacy references and a rollback binary are in the ignored `build-extractors/bin/Release/fixed/2026-11-28-gilneas-endgame-gunship/`. Cleanup removed 21 generated files totaling 923,314,796 bytes. The existing mixed 1,347-byte DB error log remains available.
