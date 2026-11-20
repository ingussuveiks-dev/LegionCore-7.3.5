# Fel Surge Totem in Feltotem's Fall

Legion build 26972, map 1702, scenario 1266. The original Tugar cast
243224 selects a nearby destination and launches 237903. The latter
summons creature 121499 with native totem properties 3407 and a
20-second lifetime. Its WDB record supplies model 4590 and the mechanical
creature type. The repair adds the missing server creature template and
retains the native missile, summon and area effects.

The totem has one health point. It uses the normal eight-second 242496
cast. If it finishes, the native hostile-area effect stuns for five
seconds within 100 yards. Killing the totem before completion prevents
the effect. The totem's cast fails closed when Tugar has died, the player
has left, or the fight has reset. A missile or totem from the previous
pull cannot activate during the next one. Existing Fel Burst, Fel Shock
and the three finale credit actions retain their SmartAI paths.

The launch delay of 54.5 seconds and the subsequent 25–33-second
intervals reconstruct the July 2018 DBM combat observations. The
addon starts the first alert at about 62 seconds. Native 243224 has a
15-yard radius and speed 2 yards per second, so collision and elevation
may change the actual arrival time. The original server timer and its
random distribution were not recovered.
[DBM's July 2018 source](https://github.com/DeadlyBossMods/DBM-Challenges/blob/cf9f66ea21818a6f503644599c5c4e1b28990a00/DBM-Challenges/LegionArtifacts/DPSFelTotem.lua)

The source-extracted AddressSanitizer regression checked the encounter
entry, player-controlled target, timer boundaries, native spell hooks,
one health point, bounded failed-cast retry, killed and stale totems,
Tugar death, reset, phase and teleport rejection. The native DB2
regression verified the original spell and summon records. Applying the
migration twice in the disposable world database preserved the previous
finale rows. The first fixture import exposed two required text columns
that the migration now sets explicitly.

The staged and canonical executables started against the migrated
fixture, then shut down cleanly. The real MariaDB migration was applied
manually and read back. The final test and import runs found no new
crashes or server diagnostics. Verified local evidence is under
`build-extractors/bin/Release/fixed/2026-11-20-feltotem-fel-surge/`.

The client still needs to confirm ground placement, missile and cast
presentation, direct pet or player attacks, the resulting stun and
its cancellation on a wipe. Earthquake and Jormog's rupture, rock
collision and scale removal remain open.
