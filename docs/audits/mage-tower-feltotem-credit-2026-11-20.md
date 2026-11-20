# Feltotem's Fall credit and Discipline admission

Legion build 26972, map 1702, scenario 1266. Its LFG entry 1445 admits a
single player. Tugar and Jormog's finale actions previously used SmartAI
target 16. When a pet or guardian killed either boss, the raw killer
could not pass that selector. The two final credit casts and Jormog's
criterion therefore missed the controlling player.

The migration selects the actual invoker player or controlling player
within 100 yards, uses native spell 233870 and criterion 58088, and
retains both guards that require the other boss to be dead. The native
selection keeps the same map and phase and uses no nearby-player fallback.
Quest 45627 now admits Priest after rewarded prerequisite 45842. Both
Mage Tower teleport choices already filter for Discipline specialization
256. The Warlock, Monk and Hunter branches retain their existing masks.

The copied MariaDB regression read the original rows, applied the
migration twice in connection-local tables and verified the exact results,
both death guards and the paid/free specialization predicates. The
source-extracted SmartAI regression passed under AddressSanitizer with
the repaired cast tuple and verified the native calls in the linked
Release executable. It covered player, pet, guardian and charmer,
invalid controller, range and phase cases. The initial harness extraction
included unrelated EventObject functions and was narrowed to the credit
functions before the passing run.

The migration was explicitly imported into the configured MariaDB with the
server offline. Readback matched the four repaired rows, the retained
guards and the migration history. The owned database shut down with exit
0. The final regression and import runs found no new crashes or appended
server diagnostics. Local evidence is under
`build-extractors/bin/Release/fixed/2026-11-20-mage-tower-repair/`.

Client verification still includes each boss order, pet final blows,
same-tick deaths, exit/re-entry and appearance hand-in. Tugar's Earthquake
and Fel Surge Totems and Jormog's burrow, rock collision and scale
removal are still open.
