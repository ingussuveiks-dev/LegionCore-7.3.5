# Agatha's Fury on imp deaths

Legion build 26972, map 1616, scenario 1221. The encounter formerly cast
242879 periodically, including while no imp had died. Agatha now uses
242879, whose native missile triggers 242880, at the encounter player's
position when one of her own Fuming, Smoldering, Umbral or Servant imps
dies. Both Servant entries are covered. The player's pet or guardian
can be Agatha's current target. The death comes from the native summon
callback, which runs after the summon has entered the dead state. A
live despawn, an unrelated summon or the Fire Mage acquisition on the
same map cannot trigger the callback's cast.

The original Fury Warrior encounter account describes the spell on
imp death and distinguishes it from the boss's shield channel.
[Legion Fury Warrior encounter reference](https://www.wowhead.com/guide/fury-warrior-mage-tower-challenge)

Agatha retains her SmartAI behavior for all remaining events. The
connection-local SQL regression applied the migration twice, checked
that only the old timer was retired, and verified the native victory
and Umbral shield cleanup rows. The AddressSanitizer regression ran the
extracted production callback for all five imp entries and player,
pet and guardian targets, with 16 rejection cases. The actual Release
link uses the scripts library containing the callback. Staged and
canonical startup against the isolated, migrated MariaDB fixture
reached readiness and exited cleanly, with no new diagnostic errors.
The live migration was applied manually and read back. The owned
database exited with code 0. Evidence and hashes are under
`build-extractors/bin/Release/fixed/2026-11-20-mage-tower-agatha/`.

The client must still confirm the missile telegraph, dodge window,
overlapping imp deaths and the encounter's full progression. The 50%
and 35% relocations, Fel Boulders, lava and full imp-wave cadence remain
open. The parallel Xylem investigation found only one native teleport
definition for destination 1406, spell 229196, shared by the paid and
free routes. It has no native currency reagent. The paid route still
needs a charge tied to an accepted transfer.
