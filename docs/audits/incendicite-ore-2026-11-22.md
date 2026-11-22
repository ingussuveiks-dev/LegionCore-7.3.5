# Incendicite Ore 25735

Dunlor Marblebeard `41128/119136` offers the level-18 quest after Fenbush Berries `25725`. The original Spark-Proof Pick `55240` is a reusable quest item. Its native spell `77020` opens lock `1896` on the selected Incendicite Mineral Vein `203188`. The Wetlands data contain 12 matching veins. Each native loot table grants one Incendicite Ore `55241` to an eligible character. The two old mineral templates with the same name retain their own lock and loot data. The entire 255-second 2016 walkthrough was sampled at five-second intervals and shows the route into Thelgen Rock, the vein and loot interactions, and the return to Dunlor. [2016 walkthrough](https://www.youtube.com/watch?v=KkADD_pJZNA)

The active accepted quest and carried pick gate both the marker and native lock. The spell's nearby-object target resolves to the originally selected vein. The loot path uses the ordinary chest handling after the lock succeeds. Quest loot admission closes after collecting eight ores. The player returns the pick and eight ores to Dunlor. Abandonment removes the pick and the ore already gathered.

## Server verification

- The pinned `ed507d0b0` game library reproduced a direct empty loot window for an inactive player and left three genuine quest ores behind after abandoning.
- The native regression verified the prerequisite, the 12 original placements, rejection of an inactive player and a stale pick, rejection of the older same-name template, three native loots followed by abandonment, eight loots during a new attempt, and Dunlor's real class-package reward packet. The controlled player had no Mining skill.
- Retained Thelgen Seismograph, Liberation Day and the four supply-chest regressions passed. The actual final link consumed `game.lib` SHA-256 `97FB764D59A4F97B558360310445EFC53CFF8BBABDD522C8801AE371F5CCA060` and `scripts.lib` SHA-256 `FAA1829A165D9A6C90272CFF2C8F06D2A8B32EDCBF06AA6473208DDFA04B3303`. The final map and PE match `worldserver.exe` SHA-256 `AAC14D81E98E1739FD2B57D261128A88CE6F7048FAED81AAE87E29A57729254D`.
- World migration `2026_11_22_38_incendicite_ore.sql` was applied after the matching executable and registered as `RELEASED` with SHA-1 `910B170B005F9E2A01D1C1833381794C71C0992E`. Production readback matched the private test data. Staged and both canonical starts reached readiness and shut down cleanly. No check reported a new crash, unexplained warning or DB error.
- Hash-verified diagnostics for 163 files are in `build-extractors/bin/Release/fixed/incendicite-ore-2026-11-22/`. Cleanup reclaimed 2,177,982,953 bytes. The previous executable and map remain in the local `rollback` directory.

Actual client cave navigation, the mining animation and loot display, and the reward interface remain open.
