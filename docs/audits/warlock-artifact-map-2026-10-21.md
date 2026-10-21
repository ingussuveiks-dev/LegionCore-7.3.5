# A Greater Arsenal remains on the map, 2026-10-21

## Observed client failure

After the Affliction acquisition filter fix, the user confirmed Calydus's
artifact selection was gone. Two screenshots show an empty Calydus conversation
but the Dreadscar Rift map still offers **A Greater Arsenal (44089)**. This is
a separate map-availability path, not a remaining artifact-choice response.

Read-only character queries found rewarded 40623, 42125, 43254 and 43984, with
44089 neither active nor rewarded. The world has no creature queststarter for
44089 and no Source-19/20 condition for it; Calydus is its questender. Its native
QuestLineXQuest entries independently advertise the breadcrumb:

| Entry | Quest line | Order |
| --- | --- | --- |
| 3465 | 247, Weapon of Legend | 23 |
| 3467 | 207, Council of the Black Harvest | 4 |
| 3888 | 274, The Tome of Blighted Implements | 2 |

Player::UpdateAvailableQuestLines calls CanTakeQuest for these records, which
calls SatisfyQuestConditions (Source 19). The gossip and choice conditions did
not cover that source. Thus the map could still advertise the unfinished
breadcrumb after its purpose had already been fulfilled.

## Fix and checks

Migration 2026_10_21_03 copies choice 245's scoped unfinished-route/class
predicate to Source-19 quest 44089, preserving its AND groups and OR alternatives.
All three acquired blocks the breadcrumb; an unfinished acquisition retains
the route. This uses quest history, not item presence. No character quest is
granted, reset or marked rewarded, and no item changes are made. Other quests
and gossip retain their existing conditions.

Test-WarlockArtifactChoice.ps1 passed with the actual production map-update,
CanTakeQuest, condition admission, quest-status, level, class and race functions
as well as the existing display and response handlers. Unrelated admission
gates are stubbed in this focused fixture; an explicit blocking gate is also
checked. Only the target quest and an unrelated test line's quest templates
are loaded, so this is not a full simulation of every quest in those chains.

The native DB2 reader verifies all three real records and passes their IDs into
the map-update test. All 64 complete/rewarded route combinations pass; the
reported history reproduces the old map offer with an empty gossip menu when
Source 19 is removed, then excludes it when restored. Checks also cover stale
dynamic-field clearing, low level, another class, already active/rewarded
44089, another failing prerequisite and retention of an unrelated map line.
Existing choice and abandoned-route tests remain passing.

Repeated temporary migrations preserve the 42 choice conditions, produce 15 map
conditions and preserve unrelated condition rows. The scoped pre-migration
backup is ignored under .codex/backups/warlock-artifact-map-20261021. Live SQL
was applied transactionally and verified as Source 19, entry 44089, with group
counts 9/3/3. No executable rebuild or replacement was needed.

The user stopped their worldserver. An owned hidden validation server (PID
32264) loaded the new conditions, reached ready at 14:22:52 and shut down
gracefully with exit 0 after database closure at 14:22:56. New startup log
lines contained no ERROR; DBErrors.log and scoped
stderr were empty. Logs are under ignored runtime-test/warlock-artifact-map.
No validation worldserver remains running. Client verification of the map
after the next server start and character login remains pending.
