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

The map 1280 guards are Alliance Guard `84557` in Southshore and Tarren Mill Deathguard `84593` in Tarren Mill. Their archived creature records and the public NPC catalog identify both entries. The catalog ties both to the battlefield zone `7107`, with an empty per-NPC coordinate list. The live world database, the project’s base 26972 world dump, the archived TDB 7.3.5 world dump and the local WoD spawn archive contained no placements for these entries on map 1280. The prior Brawl setup spawned only spirit guides. Blizzard’s June 2 hotfix confirms stronger, faster-respawning guards and one team point per enemy NPC kill. [Alliance Guard catalog](https://www.wowhead.com/ptr/npc=84557/alliance-guard), [Tarren Mill Deathguard catalog](https://www.wowhead.com/npc=84593/tarren-mill-deathguard), [Blizzard’s June 2 hotfix](https://worldofwarcraft.blizzard.com/en-gb/news/20653889/hotfixes-april-17).

### Guard sightings in Legion footage

| Town | Visible placement | Footage |
| --- | --- | --- |
| Southshore | Two armored, sword-bearing figures linger by the fork in the cobbled road on the way from the docks. One is at the foot of the stone stairs beside the round stone fixture. The other is across the road, on the grass by the timber building. The same two positions are visible as the camera moves, with the nearby battle farther up the road. | [Swifty, 7.3, 7:18–7:30](https://www.youtube.com/watch?v=toqn191n8OA&t=438s) |
| Tarren Mill | Two sword-bearing figures are visible around the fenced orchard beside the timber building. One is just inside the near corner of the orchard fence. Another passes on the grass outside the fence on the building side. A shield-bearing figure is also visible at the corner of the timber building at 6:06. The footage captures movement beside the fence. | [Game On, Legion 7.3.5, 6:03–6:12](https://www.youtube.com/watch?v=Slyl9t4ch6M&t=363s) |

The Legion footage identifies landmarks by visible props, armor and weapons. The nameplates of those figures are unreadable. The 2014 anniversary fight shows the readable Tarren Mill Deathguard name on the orange-shield model beside the same approach through town. This corroborates the guard identification in the Legion footage. Combat movement in the anniversary recording cannot locate a spawn. [2014 anniversary fight, 4:24–4:36](https://www.youtube.com/watch?v=zPy3gVjALeg&t=264s).

The videos do not show spawn XYZ, orientations, complete guard counts or patrol waypoints. Build 26972 map 1280 has these reference points: Alliance start `1834.07, 1077.64, 12.054` (4848), Southshore Town Hall `1790.08, 1079.05, 12.4768` (233964), Horde start `2671.62, 744.132, 58.7772` (4849), and Horde graveyard `2654.19, 619.064, 55.8535` (4851). The reference points locate the towns and the respawn, and the sightings locate the guards relative to nearby buildings and paths.

Review covered the full runtimes as sampled frames: 11:40 from Swifty (6-second intervals), 11:27 from the February 2018 warlock recording (6-second intervals), 26:28 from Koshmar’s Horde match (15-second intervals, with the opening sampled every 4 seconds), 11:17 from the 7.2 Alliance match (8-second intervals), 2:05 from the 7.2 PTR clip (3-second intervals), 34:24 from the 7.2.5 compilation (15-second intervals), and 18:19 from the 2014 anniversary fight (12-second intervals). Relevant scenes were also inspected at source resolution. This was a frame review without audio or continuous playback. [Koshmar’s Horde match](https://www.youtube.com/watch?v=58zb1Bi77mQ), [7.2 Alliance match](https://www.youtube.com/watch?v=qMtLufQGnWc), [7.2 PTR clip](https://www.youtube.com/watch?v=6JJ6ifECjak), [7.2.5 compilation](https://www.youtube.com/watch?v=HcmNY7VF6xs).

Additional searches of the project’s archived WoD and Legion update sets, the local TrinityCore SQL tree, and the two local TDB 12.x world dumps did not recover a matching guard spawn row.

### Restored guard placements

| Town | Placement on map 1280 | X | Y | Z |
| --- | --- | ---: | ---: | ---: |
| Southshore | Beside the well and stair foot | 1815.50 | 1087.00 | 12.2660 |
| Southshore | Across the road toward the timber building | 1831.00 | 1063.00 | 13.4681 |
| Tarren Mill | Inside the orchard fence | 2585.00 | 712.00 | 55.1379 |
| Tarren Mill | Patrol between the farm corner and the outer orchard fence | 2605.25 | 699.00 | 55.4913 |

The patrol turns at `2595.50, 703.75, 55.4762`. The well model origin, orchard fence and farm building footprint were matched against the extracted 26972 collision data. Positions, count, orientation, equipment, patrol and the 30-second respawn delay plus a five-second corpse timer were reconstructed. The level-110 guards use the existing elite creature multipliers and native faction relations. The battleground owns all four guards and ties credit to their instance GUIDs. A qualifying enemy kill grants one team point through the existing 150-point finish, including a kill credited to a player by a pet or guardian. The stationary guards use their home positions on respawn. The patrol returns to its home in the patrol area and restarts its native waypoint movement.

The score fixture passed for both teams, owned and foreign units, friendly kills, uncredited kills, and guard kills reaching 150. The native death dispatch was checked separately for once-per-death entry and player ownership. A map probe verified all five terrain heights and both directions of the patrol with the extracted collision and navigation data. The native faction rows confirmed friendly and hostile relations for both baseline faction templates. The SQL migration was stable on a second application in an isolated database, and the live updater recorded it as `RELEASED`.

The canonical Release server reached ready and shut down cleanly. Its new log output had no warnings, errors or assertions, and no new crash or `DBErrors.log` entries appeared. Prior diagnostics were retained. The match itself, including combat, respawn timing and mixed-faction presentation, remains to be checked in the client.
