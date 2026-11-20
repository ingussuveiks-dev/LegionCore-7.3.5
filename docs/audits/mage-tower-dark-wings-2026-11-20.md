# Sigryn's Dark Wings

Legion build 26972, map 1703, scenario 1265. Sigryn now summons the
native Dark Val'kyr 119696 through 237730. Her voice and raid emote
use the original broadcast text 128424 and 128428. The Val'kyr use
the three-second preparation spell 237769, the movement spell 237759
and the native 237761 damage selector. The gap remains clear of
charge lanes. The same target can be selected once per wave, even on
the boundary of two adjacent six-yard lanes. Participants and their
controlled units are eligible for the original hostile-area effect.

The formation uses eight actors across ten lanes and omits two adjacent
lanes. These lane counts, placement and repeat interval reconstruct the
original behavior using the observed gaps and the native six-yard damage
width. The installed geometry supports three complete cross-arena axes
and their reverses. The original spawn coordinates and server scheduling
table were not recovered. The first wave is scheduled at about 146 seconds
and repeats at a 20-second interval based on the Legion DBM warning.
[July 2018 DBM reference](https://github.com/DeadlyBossMods/DBM-Challenges/blob/cf9f66ea21818a6f503644599c5c4e1b28990a00/DBM-Challenges/LegionArtifacts/DPSQueen.lua)

The 2018 Demonology guide was reviewed visually across 0:00–8:21 at
five-second samples. Full-resolution frames at 5:49 and 6:49 showed the
warning alongside Ancestral Knowledge and the assembled Val'kyr line
with its gap. The capture is a guide with cuts and overlays, so it does
not establish every interval of an uninterrupted encounter.
[Original Legion Demonology video](https://www.youtube.com/watch?v=zPM2NCbILLE)

A partial formation is removed before it can charge. Prepared Val'kyr
validate Sigryn, the live scenario, the participant and the wave at
release. Reset, surrender and player loss invalidate the wave. The
existing Blood of the Father, shared health and victory paths remain
in Sigryn's SmartAI.

The native catalog check verified the summon, preparation, movement
and damage records. AddressSanitizer ran the production formation
and release code for all 42 direction/gap pairs, a missing summon,
a failed charge, a stale caster and a repeated callback. The VMAP
probe verified the installed arena geometry for the selected axes.
Applying the migration twice in isolation preserved Sigryn's original
SmartAI rows and the earlier Faljar and Twins bindings. The staged
and installed Release builds reached readiness and shut down cleanly.
The migration was also manually applied to the real MariaDB and read
back. No new crash reports or diagnostic errors were found.
Local evidence is under
`build-extractors/bin/Release/fixed/2026-11-20-mage-tower-dark-wings/`.

The 7.3.5 client still needs to confirm model orientation, the warning
and charge presentation, movement into the gap and actual damage.
These checks do not certify the whole council encounter.
