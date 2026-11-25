# Legion 7.3.5 arena rotation and Southshore guards

Client build 26972 lists eight maps for the standard arena queue. The world database has a template with selection weight 1 for each map. Every pair of starting locations resolves to that map, and each map has a level-110 bracket, extracted terrain, collision and navigation data. The live map, battleground and LFG disable lists contain no rows. Arena season 25 is active. All eight arena classes resolve their door, buff and other required spawn templates from the world database.

| Arena | Map | Template | Alliance / Horde starts |
| --- | ---: | ---: | --- |
| Blade’s Edge Arena | 1672 | 844 | 5816 / 5817 |
| Dalaran Sewers | 617 | 10 | 1362 / 1363 |
| Nagrand Arena | 1505 | 809 | 5126 / 5127 |
| Ruins of Lordaeron | 572 | 8 | 1258 / 1259 |
| The Tiger’s Peak | 1134 | 757 | 4534 / 4535 |
| Tol’viron Arena | 980 | 719 | 4136 / 4137 |
| Ashamane’s Fall | 1552 | 816 | 5307 / 5308 |
| Black Rook Hold Arena | 1504 | 808 | 5121 / 5122 |

The Ashamane’s Fall setup had both Shadow Sight objects at the northeast placement. The first object and both roots match the capture in TrinityCore commit `39d8f96fdf`. The second object's recorded southwest placement is `3503.2822, 5502.1509, 325.70718`. The fix uses that placement with its captured rotation and keeps the local Legion object entries and timing. The coordinate check fails on the old duplicate and passes on the repaired setup. The compiled queue regression deterministically selects all eight weighted entries. Live client entry, gates, buffs, combat and full matches remain unverified. [TrinityCore capture of Ashamane’s Fall](https://github.com/TrinityCore/TrinityCore/commit/39d8f96fdfc53d43e998edaa25ec3711d1a047e1).

Legion supports rated 2v2 and 3v3. Blizzard retired the rated 5v5 bracket in the Legion pre-patch. Skirmish uses the arena rotation with team sizes assigned by its matchmaker. [Blizzard’s Legion patch notes](https://worldofwarcraft.blizzard.com/en-us/news/20151961).

## Southshore vs. Tarren Mill

The map 1280 guards are Alliance Guard `84557` in Southshore and Tarren Mill Deathguard `84593` in Tarren Mill. Their archived creature records and the public NPC catalog identify both entries. The catalog ties both to the battlefield zone `7107`, with an empty per-NPC coordinate list. The live world database, the project’s base 26972 world dump, the archived TDB 7.3.5 world dump and the local WoD spawn archive contain no placements for these entries on map 1280. The matching client data identifies the faction starts and town landmarks. The current Brawl has only spirit guides, so guard locations, count, tuning and NPC scoring remain open. Blizzard’s June 2 hotfix confirms stronger, faster-respawning guards and one team point per enemy NPC kill. [Alliance Guard catalog](https://www.wowhead.com/ptr/npc=84557/alliance-guard), [Tarren Mill Deathguard catalog](https://www.wowhead.com/npc=84593/tarren-mill-deathguard), [Blizzard’s June 2 hotfix](https://worldofwarcraft.blizzard.com/en-gb/news/20653889/hotfixes-april-17).
