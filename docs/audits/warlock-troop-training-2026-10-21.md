# Dyala and Recruiting the Troops (41797), 2026-10-21

## Observed failures and native evidence

The client showed three identical "I would like to requisition some Imps"
options at Imp Mother Dyala 106216. Menu 19918 contained identical service-28
rows 0/1/2; none chose a different shipment. The read-only character snapshot
had active 41797, no troop followers yet and three saved shipment-255 orders.

The previous service fixture had misclassified the records from the flag name
without checking duration/capacity. The native 7.3.5 reader now establishes:

| Shipment | Duration | Queue capacity | Follower | Spell / completion |
| --- | --- | --- | --- | --- |
| 321, first training | 5 seconds | 1 | 649 | 219621 / 219732 |
| 255, ordinary imps | 1800 seconds | 4 | 649 | 210565 / 210566 |
| 300, Dark Mastery | 1800 seconds | 4 | 738 | 215673 / 215674 |

All three cost 20 Order Resources (currency 1220), have DummyItemID 0 and use
completion effect 90 for credit 106208, the actual 41797 objective. Flags are
0/1/1 respectively. The generic selector chose the slow record while 41797 was
incomplete and the fast record after reward; its Warlock upgrade branch also
tested unrelated follower 681. This was a real progression defect, not just
duplicate wording.

## Fix

Scope the selector to Warlock container 129 / required quest 41797: incomplete
selects 321; rewarded selects 255 or 300 with talent 362; absent/complete/failed
does not start another tutorial. Explicit configured ShipmentID and other
containers retain the existing logic. Missing native records fail closed.
Migration 2026_10_21_04 removes only exact copies of option 0 at IDs 1/2.

Saved orders are not deleted or rescheduled. They still finish on their saved
timestamps and grant the normal follower/credit on collection. In particular,
the user's three old 30-minute orders remain queued; a new fast order cannot
be added while they exceed the tutorial's one-order capacity. Collect the
first completed Imp Shipment (GO 250912 beside Dyala) to get the objective.
Merely queueing the order does not award 41797 credit.

## Checks and deployment

Test-WarlockTroops.ps1 passed all six native-record iteration orders, actual
selection, effect-216 creation, PlaceShipment timing, ready updates, the full
item-free collection path, effect-90 credit and native queue deletion. It checks
early/repeated clicks, saved slow orders, normal/upgraded variants and native
currency data. It does not simulate resource charging, every packet or an
actual client click. SQL tests repeat the repair and preserve unrelated menus
and a distinct payload at one of the removed IDs. The existing native gossip
service/adventure-map tests also passed with corrected shipment expectations.

Original-path game.lib was rebuilt and the Release executable linked with only
its OutDir staged. Canonical EXE/MAP were backed up and replaced while no user
worldserver was running; configs, DLLs, certificates, data and logs were kept.
World SQL backups are ignored under .codex/backups/warlock-troops-arsenal-20261021.
Canonical SHA-256: `0c45963608368fa13e92f7189ec6b7a668844a6b6fa930f534f7797e8472920b`.

The PE/unwind/thunk check verifies native selector gates, 321/255/300 constants,
talent 362 and calls from all four shipment service paths. Scepter login Release
verification, ten scenario-LFG paths and five network diagnostic call sites
passed. The optional PE inspector requires capstone.

Owned hidden canonical worldserver PID 33456 reached ready at 14:46:50 and
exited 0 after graceful shutdown/database closure at 14:46:54. New Server.log
lines contained no ERROR; DBErrors.log and stderr were empty. No validation
server remains running. Actual client training UI, five-second order, collection,
quest turn-in and the following mission remain client-pending.
