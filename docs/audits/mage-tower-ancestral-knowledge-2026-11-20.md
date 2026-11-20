# Runeseer Faljar's Ancestral Knowledge

Legion build 26972, map 1703, scenario 1265. Faljar previously had no
caller for 237949. He now casts this native shield, which absorbs five
million damage and applies the native interrupt and silence immunities.
Its effect 160 starts 237952, the normal 20-second damaging cast. The
shield must be depleted before the player or a controlled pet can
interrupt Faljar through the normal spell engine.
[Original Legion encounter account](https://www.wowhead.com/guide/demonology-warlock-mage-tower-challenge)

Faljar waits for the current cast to finish before starting the shield.
The brief handoff gives its native triggered cast priority over the
next Shadowbolt. Once the blast is acknowledged, his original SmartAI
updates resume. A failed handoff removes the shield, records a warning
and allows a later retry. Interruption or completion cleans up Faljar's
own shield. Surrender, death and reset cancel the active cast and deny
any pending blast. The original Shadowbolt, runes, shared health and
surrender rows are retained.

The timing reconstructs the July 2018 DBM observations. The first
cast begins around 98.4 seconds and successive intervals shorten late
in combat. A later DBM fix registers spell 237952 as a cast-start event
and restores the warning timer that was assigned to Bladestorm in the
older code. The original server's scheduling and interactions with
other council abilities were not recovered from these observations.
[July 2018 measurement table](https://github.com/DeadlyBossMods/DBM-Challenges/blob/cf9f66ea21818a6f503644599c5c4e1b28990a00/DBM-Challenges/LegionArtifacts/DPSQueen.lua)
[DBM cast-start event correction](https://github.com/DeadlyBossMods/DBM-Challenges/commit/0001878b8430781f29a6fd3a167abb0bda809ba2)

The native catalog check matched the shield, immunities, normal cast
time and damage. The source-extracted AddressSanitizer regression
exercised timing, coexistence with Shadowbolt, rejection before a
real shield attempt, pet targeting, interrupted-cast cleanup, failed
handoff and retry, surrender, death, reset and out-of-scope cases.
The disposable MariaDB migration was applied twice and preserved all
12 Faljar SmartAI rows and the earlier Fel Surge Totem bindings. Both
the staged and deployed build reached readiness and shut down cleanly
with the migrated fixture. The migration was also applied manually to
the real MariaDB and read back. The checks reported no new crashes or
diagnostic errors. Local evidence is under
`build-extractors/bin/Release/fixed/2026-11-20-mage-tower-faljar/`.

Actual shield absorption, interrupts from the player and pet, spell
presentation, completed damage and the full fight remain client checks.
Dark Wings and the council's encounter-wide timing remain open.
