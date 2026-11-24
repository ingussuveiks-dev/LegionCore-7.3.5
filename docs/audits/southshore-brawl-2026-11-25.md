# Southshore vs. Tarren Mill rotation and match scoring

The existing Brawl was absent from the rotation and live templates. Queue 789 now uses build 26972 map 1280, faction starts 4848 and 4849, and resurrects through 4850 and 4851. A duplicate Arathi Blizzard rotation slot now offers Southshore. The native minimum is ten players per side, with up to forty.

Enemy player deaths award one team point and broadcast the updated score. The first team to reach 150 wins. A timeout uses the current score. Rank spells use honorable kills, including the engine's eligible nearby credit, and remain bounded. The rank aura and count clear on death or leaving. Environmental, friendly and unrelated-player deaths award no team point. The registered kill path uses the core's owner resolution. [Blizzard's Southshore scoring hotfix](https://worldofwarcraft.blizzard.com/en-us/news/20653889/hotfixes-april-12).

The regression exercised the production combat methods through controlled player and scoreboard endpoints. It covered null killers, friendly and out-of-match killers, rank boundaries, death cleanup, and ending at 150 rather than awarding a later kill. MariaDB applied the template twice in a shadow and produced one row. The final Release linked the intended Brawl implementation and the installed executable SHA-256 is `5787DD9EED27042586272007692846E85221DA4C748EB0A815468740896FAF9A`. The owned canonical startup registered migration 01, loaded 44 templates and exited normally with no new warning, DB error or crash.

The local map currently has no static battlefield guard placements. NPC scoring, town defenses and start barriers still need authentic placement and behavior evidence. No multiplayer match, scoreboard rendering or spirit release was tested in the client.
