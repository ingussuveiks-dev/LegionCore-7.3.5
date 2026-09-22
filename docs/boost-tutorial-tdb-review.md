# Boost tutorial: full TrinityCore database review

Research date: 2026-09-22. Read-only extraction; no SQL imported and no
characters moved. These are candidate records, not an installable migration.

## Sources

- Local TrinityCore checkout: `C:/wamp64/www/TrinityCore`, commit
  `18b50b492db6e84a2160c1e1555d0d9e63a7106a`.
- Local full modern world dump:
  `C:/wamp64/www/TrinityCore/sql/TDB_full_1210.26091_2026_09_09/TDB_full_world_1210.26091_2026_09_09.sql`.
  SHA256: `608534D1DC97535C92D756914A02FE36264E484B519F6E54F6D10CF56C6F4D96`.
- Official historical release: https://github.com/TrinityCore/TrinityCore/releases/tag/TDB735.00
  Asset: `TDB_full_735.00_2018_02_19.7z` (103925362 bytes).
  Extracted world dump: `TDB_world_735.00_2018_02_19.sql` (313674890 bytes).
  SHA256: `786D8B7F84FE1DAF765B770483721AE529F3D758DA2271F812C261D914F1E99E`.
  Download/extraction directory:
  `C:/Users/Admin/AppData/Local/Temp/legion-tdb735-review-037e32914e4f4734b20c75dced3d97b8`.

## Method and scope

`tools/research/Read-TdbBoostEvidence.ps1` streams the SQL without executing it.
It selects known tutorial NPCs, transport objects 246606/246607, maps
1554/1557/1559/1560, gossip menus 19768/20085/20459, corresponding text IDs,
and six start/exit spells. It also checks selected creature texts, SmartAI,
instance bindings, transport registrations, vehicle accessories and spell
bindings/target positions. The exact ID set, scanned row counts and schemas
are preserved in the script and JSON outputs.

The `creature` and `gameobject` first three columns were verified as
`guid,id,map` in both dumps. This is a scoped search, not a proof that no
related data exists anywhere in either archive. Hotfix dumps and bundled
historical update dumps were not exhaustively scanned here. Arbitrary
coordinates embedded in C++ or alternate spell/creature IDs remain outside
this extraction's scope.

## Results

| Check | TDB 735.00 | TDB 1210.26091 |
| --- | --- | --- |
| Total creature spawn rows scanned | 206488 | 262623 |
| Total gameobject spawn rows scanned | 76672 | 104080 |
| Selected creature templates | 33 | 33 |
| Matching creature/gameobject spawns | 0 | 0 |
| Selected gameobject templates | 246607 | 246607 |
| Selected instance templates | 1554, empty script | none |
| Selected gossip menus/options | none | 3 menus, 3 options |
| Selected creature text / SmartAI / spell bindings | none | none |

Both contain The Warbringer (246607), transport path 5529, interior map1559,
VerifiedBuild22566, consistent with the local Legion data already inspected.
Neither selected result contains The Dawn's Blade template246606. Neither
contains the selected transport registrations or vehicle accessories.

The historical NPC templates have VerifiedBuild25549; key trainers/exit
vehicles have empty AIName/ScriptName. Presence of names, models and template
fields does not establish working NPC behavior or even correct runtime defaults.

The modern database retains gossip for both exit vehicles, but menus20085 and
20459 now say **leave for the assault on Lordaeron**, BroadcastTextID158312,
with builds27843/27219. The locally inspected Legion menu uses Broken Shore
text118578 instead. This is concrete evidence against copying modern rows
wholesale into `legion_world`. Modern VehicleID4933 also needs validation
against the local7.3.5 DB2 before any use.

## Evidence files

- `boost-tutorial-tdb735-evidence.json`: 35 selected historical rows.
- `boost-tutorial-tdb-evidence.json`: 45 selected modern rows.
- `boost-tutorial-followup.md`: existing DB2, local server and video findings.

## Conclusion

The user's suggestion was useful: full release databases retain reference
data that a source/update-only search can miss. However, neither tested full
world dump supplies the missing spawn coordinates and complete tutorial logic.
The map1554 entry with an empty script is not a functioning scenario.

Still needed: verified transport-local placement/boarding, encounter and wave
logic, surrender/reset behavior, class progression wiring, and safe exit and
reconnect handling. Historical packet captures or a demonstrably implemented
server script plus its matching SQL remain the strongest leads. Keep the
existing safe boost destination until those pieces are implemented and tested.
