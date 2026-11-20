# Rune of Summoning in Thwarting the Twins

Legion build 26972, map 1684, scenario 1273. Raest now starts the rune
sequence at the existing DATA 5/5 transition following Karam's second
Purgatory. The sequence can continue into the final phase. It uses the
original warning 236468 and 236469 → 236705 → 236460 missiles and area
trigger. Trigger 9778 lasts eight seconds. A private server script key
based on spell 236460 avoids sharing the script with the unrelated
193998 area trigger, whose custom key is also 9778.

Three continuous seconds of occupancy by the encounter player or an
eligible controlled pet or guardian disrupt the rune. Moving out breaks
the occupancy. A rune that expires without a soak uses native spell
236461, properties 3499 and creature 119098. It produces one Nightmare
that pursues the encounter player. A reset, loss of the participant or
Raest's death removes the rune and its summons. Each missile and rune
validates its originating attempt. Raest and Karam retain the existing
phase, credit and victory actions.

The original cast records establish the warning, spell sequence,
duration and summon. The 2018 DBM script supplies the approximate
18.2-second and 35-second timing anchors. The repair starts its first
cast in the second Raest window because original Legion footage shows
the warning and soak while Karam is still in Purgatory. The Frost Mage
guide was reviewed visually across 0:00–10:31 at five-second samples
and 8:22–8:47 at half-second samples. It also shows the missed-rune
outcome and the encounter ending. The 3-yard radius, three-second
occupancy, spawn sector and melee damage multiplier are reconstructions.
The original server trigger shape and timer table were not located.
[Frost Mage Legion footage](https://www.youtube.com/watch?v=2SmfPyI0C9I)
[Legion DBM reference](https://github.com/DeadlyBossMods/DBM-Challenges/blob/cf9f66ea21818a6f503644599c5c4e1b28990a00/DBM-Challenges/LegionArtifacts/DPSTwins.lua)
[Original Affliction encounter account](https://www.wowhead.com/guide/affliction-warlock-mage-tower-challenge)

The native geometry probe checked 41 sampled angles against the
installed terrain, VMAP and Detour navigation data. The DB2 check
verified spell links and durations. AddressSanitizer exercised the
production area-trigger callback for player, pet, guardian, wrong
owner, exit and re-entry, wipe, old attempts, exact expiry boundaries
and a single Nightmare. The SQL regression applied the change twice
to an isolated world database and verified Raest's phase and credit
rows and the unrelated trigger. The first build exposed that the pet
container holds GUIDs, which the final code resolves through the
native accessor before testing occupancy.

Staged and canonical starts with the migrated fixture reached
readiness and exited cleanly, without new crashes or diagnostics. The
real MariaDB migration was applied manually and checked after import.
Local evidence is under
`build-extractors/bin/Release/fixed/2026-11-20-mage-tower-summoning-rune/`.

Client verification still includes the ground visual and edge, actual
pet movement, Nightmare damage and crowd control, and a full run under
each specialization. The two Purgatory thresholds, other add cadence
and remaining Mage Tower encounters were outside this rune repair.
