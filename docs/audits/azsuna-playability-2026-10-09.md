# Azsuna playability: first repair, The Scythe of Souls

This pass inventories Azsuna's quest catalog, starter/ender actors and objective
targets, then traces the opening Faronaar branch. It does **not** certify all
Azsuna quests or the full 100–110 campaign. Dynamic summons must be traced before
treating a missing static spawn as a blocker. Client playthrough is still needed.

## Confirmed defect and repair: quest 37660

The live world database and the bundled LegionCore base dump have no objectives
for **The Scythe of Souls**. The server loads objectives from `quest_objectives`;
its DB2 objective fallback is commented out. Consequently this quest could finish
without playing its encounter, while its scripts depended on nonexistent stored
objectives. The local 26972 `QuestObjective.db2` also has no rows for this quest.

The **TDB 7.3.5 archive dated 2018-02-19**, with objective records verified at
build **25549**, supplies the original ten IDs, storage indexes, targets, amounts
and flags. Migration 08 restores those exact fields, retaining the optional exit.
`azsuna-scythe-native-2026-10-09.json` records the archive hash and native DB2
evidence. No replacement quest, target ID or objective count was invented.

Other defects in the old interaction code:

- Both gems scheduled credit for the first demon. The second had a separate
  unguarded timer in Allari's AI; a timer could credit a missing demon.
- Summon failures still awarded release credit; nearby creatures could belong
  to another player, and release order was not enforced.
- Allari added personal visibility to the summoner rather than herself.
- `MovePath(90401)` reads `waypoint_data`, which has no such path. The three
  positions exist only in the separate SmartAI `waypoints` table.
- The central entry gem 237017 is absent. The exit and old static Arev'naal are
  in mask 2, unlike normal players. The native screen spell is not a phase aura.
- The starter's unconditional proximity timer granted the final return credit
  before the interrogation or combat.

The replacement retains the existing guide and gem script names. It moves the
personal guide to the existing meeting position, credits actual arrival and
conversation, and requires the first interrogation before the second. Each
interrogation needs a successful 178939 spell hit and a live, nearby soul through
the dialogue. Failed summons, missing targets, duplicate clicks and interrupted
attempts cannot complete it. Saved release progress can be retried after a failure.

The central gem opens only after both interrogations and summons a personal
Arev'naal (89673). The normal engine kill path supplies his objective; spawning,
entry and timers do not. Returning to the original Allari within range and line
of sight only counts after victory. Death, abandonment, logout or leaving the
area cleans up personal actors and the screen aura. A failed attempt can restart
at the next unfinished objective. Old-map summons also have finite lifetimes.

The entry gem is a **recovery placement**, ten yards beside the existing exit
anchor, avoiding overlapping models. The encounter uses native screen effect
179183/1093 via `AddAura`, avoiding obsolete triggered spells. The personal
Arev'naal uses basic combat: this is executable recovery, **not** a claim that all
retail abilities, dialogue timing, inner-world scenery or cinematics are restored.
Previously rewarded characters and existing completed quest logs are not reset.

The quest's documented sequence corroborates the interaction order:
[The Scythe of Souls](https://www.wowhead.com/quest=37660/the-scythe-of-souls).
Modern rewards/levels are not imported; the objective data comes from 7.3.5 TDB.

## Verification and remaining scope

- `Test-AzsunaScytheNative.py` checks all ten migration records against the 7.3.5
  archive, plus the native summon, compel, screen effect and questline identities.
- `Test-AzsunaScytheData.ps1` applies the real migration twice in isolated temporary
  tables, checks its exact objectives, bindings, entry/exit and preservation of
  unrelated records and custom data.
- `Test-AzsunaScythe.ps1` compiles the complete production script against a small
  world boundary. It exercises ordering, ownership, range, actual-hit requirements,
  failed/duplicate summons, premature clicks, interrupted targets, retries,
  death/abandon/logout/map/area cleanup and final return restrictions. The normal
  engine kill-credit boundary is explicit, not presented as an in-game test.
- The production scripts compile in Release. Final runtime startup is checked
  separately during deployment. Actual client playthrough and visuals remain open.

The custom level-100 boost, ship/bird flight, pets, scenario criteria and warlock
handlers are unchanged. Further Azsuna work must continue through the remaining
Faronaar interactions, Azurewing Repose, Nar'thalas and Oceanus chains; an inventory
or a valid quest link alone does not establish playability.
