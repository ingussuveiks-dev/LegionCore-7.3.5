# Agatha's Fel Lava Blast

Agatha now uses the native Fel Lava Blast missile `235259` after her first completed Translocate. It aims at the active player's grounded position at the time of the cast. The native impact `235261` supplies the direct damage and creates area trigger `9645` for ten seconds. The encounter uses the original `235263` one-second Fire damage while the player or their controlled unit is inside a live patch. Leaving the last patch clears the encounter aura. Reset, player loss and Agatha's real death clean up their own lava. The damage and ground effect reject delayed impacts outside the active encounter. The Fuming Imp's own Plague Zone remains in the original data. [Original 2017 Fury kill](https://www.youtube.com/watch?v=R4a2esU-LCk), [Legion Fury encounter account](https://www.wowhead.com/guide/fury-warrior-mage-tower-challenge).

## Verification and limits

- Build 26972 data verified the missile chain, impact and area effect, effect period, six-yard radius and ten-second ground lifetime. The source-extracted AddressSanitizer check exercised live and expired areas, overlapping patches, range, height, phase and map. The earlier Translocate regression passed again.
- Both isolated MariaDB applications and manual production import produced the expected spell script and ground template. Readback retained the Fuming Imp shape, the Translocate routes, and the original victory rows.
- The staged and canonical executables reached readiness against the isolated schema and shut down cleanly. Both used SHA-256 `D91A6F3ED2C3E950D7492507BAB829D4028FEA30BDCC2A6BEE89F1C0E2B54917`. There were no new crashes, DBErrors, warnings or assertions. The verified `build-extractors/bin/Release/fixed/2026-11-21-mage-tower-agatha-fel-lava/` archive contains 37 evidence hashes and records 190,456,943 bytes removed.

The eight-second first delay and fourteen-second repeat are reconstructed from the sampled fight. Actual client telegraph, puddle display and footprint, movement, pet behavior, and reset remain pending. Felfire Boulders, Dark Fury and the full imp waves are separate checks.
