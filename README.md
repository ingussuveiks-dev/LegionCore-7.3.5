# LegionCore 7.3.5 (build 26972)

LegionCore is an open-source World of Warcraft: Legion server emulator for the
7.3.5 client, build **26972**. This repository is a maintained fork of
[The Legion Preservation Project](https://github.com/The-Legion-Preservation-Project/LegionCore-7.3.5),
which originated from the 2020 UWoW source release and an older
[TrinityCore](https://github.com/TrinityCore/TrinityCore) codebase.

This is a development and research project. It is suitable for local servers,
development and gameplay testing, but it should not be presented as a fully
complete or production-ready Legion implementation. A clean startup does not
guarantee that every quest, class mechanic, dungeon or raid encounter works.

## Current status

- Client protocol: Legion 7.3.5, build 26972 only.
- Expansion setting: Legion (`Expansion = 6`).
- Maximum player level: 110.
- Server applications: `bnetserver` and `worldserver`.
- Windows x64 Release builds are actively used and verified in this fork.
- GitHub workflows also build the project with GCC and Clang on Linux.
- Database auto-updates are supported for auth, characters, world and hotfixes.
- The repository includes the data extraction tools required by this client.

Recent work in this fork includes database consistency repairs, a full active
spell-reference audit, Antorus Tier 21 script/binding repairs, quieter and more
accurate 7.3.5 network handling, and a custom BattlePay catalogue. See the
[spell audit](docs/audits/spell-audit-2026-09-21.md) for its exact scope and
remaining gameplay-validation requirements.

## Fork-specific features

- Native 7.3.5 in-game BattlePay shop available both in game and on character
  selection, with Legion-valid mounts, pets, toys, bags, services, appearance
  items, consumables, heirloom upgrades and Antorus tokens.
- Persistent level 100 character boost entitlement with character,
  specialization and neutral-Pandaren faction selection. Delivery includes a
  specialization-appropriate starter loadout, learned progression essentials,
  riding support, destination handling, action-bar initialization and four
  30-slot bags.
- Account collection checks prevent repurchasing already-owned mounts, pets or
  toys where the client and server data allow ownership to be determined.
- Personal XP-rate NPC with selectable rates up to the configured server cap.
- Solocraft scaling code and per-instance configuration are present, but the
  feature is disabled by default.
- LegionBotAI player-character companions can join a party, follow, fight,
  heal and participate in Dungeon Finder after a dedicated bot account is set up.
- Windows dependency discovery and a Visual Studio 2022 CMake preset are
  included for the local Release workflow.

The BattlePay catalogue is a server-specific convenience feature; it is not a
claim to reproduce Blizzard's historical commercial storefront.

## Optional systems and defaults

These systems are compiled into the core but should be enabled only after
testing them with your own database and client environment.

| System | Present | Default state | Main configuration |
| --- | --- | --- | --- |
| BattlePay shop | Yes | Enabled | `Bpay.Enabled = 1` |
| Warden anti-cheat | Yes | Disabled | `Warden.Enabled = 0` |
| Extended Warden checks | Yes | Disabled | `WardenExtended.Enabled = 0` |
| AuctionHouseBot seller | Yes | Disabled | `AuctionHouseBot.Seller.Enabled = 0` |
| AuctionHouseBot buyer | Yes | Disabled | `AuctionHouseBot.Buyer.Enabled = 0` |
| Solocraft | Yes | Disabled | `Solocraft.Enable = 0` |
| LegionBotAI | Yes | Spawning disabled | `LegionBot.AccountId = 0` |

Warden requires compatible checks and careful validation before use. AHBot also
needs an account/character setup and deliberate economy configuration; changing
only one switch is not a production economy setup.

## LegionBotAI player bots

LegionBotAI is adapted from the [LrdPsychoChains WOW HUB](https://github.com/psychochaingang/LrdPsychoChains-WOW-HUB/tree/main/projects/legionbotai).
Its four-character team uses database-backed player characters: one tank, two
healers and one damage dealer. Horde uses Bulwark, Lovley, Ember and Faith;
Alliance uses Aegis, Seraphine, Rook and Elowen. The extra healer can queue as
damage for a five-player dungeon while still healing in combat.

To enable the included teams:

1. Check that character GUIDs `900000`–`900003` and `900010`–`900013`, and the
   eight names above, are unused. Back up the auth and characters databases.
2. Review and run [`sql/custom/legionbotai.sql`](sql/custom/legionbotai.sql). It
   adds a dedicated `LEGIONBOTAI` account, eight characters, their homebinds
   and the settings table without deleting existing rows. The script targets
   databases named `legion_auth` and `legion_characters`; adjust those names if
   your installation uses different ones.
3. Find the new account ID with
   `SELECT id FROM legion_auth.account WHERE username = 'LEGIONBOTAI';` and set
   `LegionBot.AccountId` to that ID in `worldserver.conf`. Keep it at `0` to
   prevent character spawning. The command accepts characters only from the
   configured account.
4. Build the Release worldserver, restart it and use `.lbot team` in game.

| In-game command | Effect |
| --- | --- |
| `.lbot team` | Spawn your faction's four player bots and add them to your party. |
| `.lbot spawn <name>` | Spawn one character from the configured bot account. |
| `.lbot dismiss` | Remove your player bots and NPC companions. |
| `.lbot info` | Show bot state and diagnostic information. |
| `.lbot level sync\|max\|<1-110>` | Match your level, use the realm maximum or set a fixed level. |
| `.lbot autogear` | Re-equip the bots for their current level. |
| `.lbot assist full\|defend\|chill` | Assist your fights, defend against attackers or avoid starting fights. |
| `.lbot follow`, `.lbot stay`, `.lbot come` | Follow, hold position or move to you. |
| `.lbot attack` | Attack your current target. |
| `.lbot aggro me\|bot` | Let you or the tank bot hold threat. |
| `.lbot self` | Toggle AI control of your own character; it uses abilities your character already knows. |
| `.lbot rescue` | Revive and teleport your character home. |
| `.lbot creatures` | Spawn a separate four-NPC companion team. |
| `.lbot tank\|healer\|dps` | Spawn one NPC companion of that role. |

The console form names an **online** player, for example `lbot MyChar team`.
The bare `.lbot` command spawns a single NPC tank companion.

The player bot spell kits were checked against the extracted 7.3.5 `Spell.db2`,
`SpellLevels.db2` and `SpecializationSpells.db2`. The included Blood DK, Holy
Paladin, Fury Warrior and Holy Priest teams learn spells for their own
specializations and current level. Removed pre-Legion party buffs are not cast;
Holy Priest no longer receives Discipline/Shadow Shield, and Fury Execute uses
spell `5308`. These four bundled specs choose one 7.3.5 talent per unlocked row
when the row is empty, preserving any existing talent choice. Holy Paladin
maintains Beacon of Light on the tank and, after its final talent unlock,
Beacon of Faith on the owner. Holy Priest refreshes Prayer of Mending on the
tank. Missing or dispelled auras are recast; Blood DK and Fury Warrior have no
general party buff to maintain. Combat uses a basic health/resource priority
and normal casts, including the GCD and resource costs. With at least three
nearby enemies already attacking the party, Blood DK prioritizes Blood Boil,
Fury Warrior uses Whirlwind for cleave and Holy Priest can use Holy Nova.
Three injured allies near a heal target enable Prayer of Healing; Holy Paladin
can use Light of Dawn for a nearby injured group. Fury Warriors learn
Titan's Grip and receive two two-handed weapons where the chosen gear is
equippable. NPC companions use separate, generic combat spells. Bot spawning,
talent application, spell effects and rotation performance still need an
in-game 7.3.5 client test.

For combat validation, compare one enemy with a pack of three already attacking
the party, then repeat with an unpulled creature nearby. Injure three party
members to check Prayer of Healing and the facing of Light of Dawn. Also check
low-level talent unlocks, Beacon renewal after dispel, resource use, threat,
target switching, follow movement, death/resurrection, looting, dungeon queue
and dismissal. The current AI does not yet handle interrupts, dispels, crowd
control or encounter ground hazards. The optional NPC companions have a
separate, simpler single-target combat script.

The Windows Release build and local auth/worldserver startup were checked on
2026-09-24: both reached `ready...`, with no startup `ERROR` or `WARN` entries,
and `help lbot` listed the command. Spawning, combat, Dungeon Finder and
dismissal still need an in-game 7.3.5 client test.

## Legion artifact and Mage Tower coverage

This fork includes class-hall artifact onboarding for all 36 Legion
specializations and the seven original Mage Tower artifact challenges. War
Councilor Victoria's gossip conditions, paid/free attempt paths and all seven
scenario entrances are present. The seven challenge maps contain their
creature, gameobject and SmartAI encounter data, and the 20 client-defined
scenario stages have progression events connected to their encounter actions.

Artifact weapon acquisition is not gated to level 110. The first weapon flow
is enabled from level 98 after the faction Legion introduction leads the
character through Dalaran to its class hall. A level 100 boosted character is
placed at the working Legion introduction hand-in and receives the appropriate
Alliance, Horde or Demon Hunter introduction quest, so it can begin the same
artifact route immediately after the boost; it does not need to level to 110
first. Once the player makes the class-hall weapon choice, the server credits
the selection and starts the matching acquisition quest for that class and
specialization. The artifact itself is awarded by the acquisition
quest/scenario rather than being granted directly by the selection handler.

All 36 specialization routes are covered. The first selected artifact can be
acquired from level 98, while quests for the remaining specialization
artifacts follow the normal class-hall progression and become available from
level 102. Characters already at level 110 can still complete any eligible
unfinished acquisition routes. Mage Tower challenge appearances are separate
endgame content and retain their level 110 and prerequisite progression.

The healer challenge, **End of the Risen Threat**, contains the shared client
criteria event `56912`, displayed as **Your allies must survive**, in five
`ANY` criteria branches. This is a survival guard rather than a missing
alternate completion trigger. Crediting it directly can advance the wrong
stage because the same event is reused by several stages. The protected allies
instead enforce it by casting the encounter's `Restart` spell on the player
when an ally dies; the world update normalizes that restart path for Commander
Jarod Shadowsong, Granny Marl and Callie Carrington.

This coverage has been checked against the 7.3.5.26972 client criteria data and
the current world database. It still requires in-game completion tests for all
seven challenge variants before it should be described as fully gameplay
verified.

## Argus campaign, world quests and Netherlight Crucible

The Argus campaign covers the Krokuun, Mac'Aree and Antoran Wastes progression,
including both mutually exclusive Antorus introduction variants. Quest `48203`
is the normal continuation after the completed campaign, while `49014` is the
fallback offered after the Netherlight Crucible finale when the normal
campaign prerequisite has not been rewarded. Both routes lead to the intended
Turalyon hand-in without allowing the two introductions to be taken together.

All 137 client-defined `QuestType = 3` tasks used by Krokuun, Mac'Aree, Antoran
Wastes and the Seat of the Triumvirate have world-quest rotation entries and
reward templates. This includes the restored special, profession, rare and
dungeon tasks as well as the Antorus introduction branch. The Alchemy quests
`48318` and `48323` retain their fixed rewards and grant the appropriate rank-2
recipe only when the player is eligible for it.

The Netherlight Crucible unlock requires level 110 and either account
achievement `12184` or completion of both finale quests `48559` and `48560`.
Relic insertion and Netherlight trait selection validate the artifact, relic
slot, trait branch and client-defined progression gates before changing player
data. The tier gates remain at artifact levels 60/63/66 and 69/72/75, and
artifact/relic changes are persisted and checked when items are transferred.

The client-data relationships, active world database and canonical Release
build have been validated. The full Argus campaign, rotating world-quest set,
reward delivery and Netherlight Crucible interaction still require end-to-end
in-game testing before they should be considered gameplay verified.

## Broken Shore building contributions

The Broken Shore contribution collector supports all three Legionfall
buildings: the Mage Tower, Command Center and Nether Disruptor. The server maps
the collector's client order index to the correct contribution, verifies the
collector NPC and client-defined player condition, and resolves the matching
hidden reward quest from the 7.3.5 DB2 data. Each accepted turn-in costs 100
Legionfall War Supplies through that reward quest, grants its configured items
and Armies of Legionfall reputation, and increments the matching personal
contribution tracker by exactly one. Full reward inventory validation happens
before the currency is removed.

The original retail accumulation target was realm-wide. For this private-server
environment it is scaled to 100 successful turn-ins per building, with each
turn-in adding one percent. A completed building follows the client-defined
lifecycle: two days Active, one day Under Attack, a short Destroyed transition,
and then a new Building cycle. Construction progress, current state and the
rotating occurrence are persisted as world states across restarts. Active
building buffs are applied from `ManagedWorldStateBuff` using their occurrence
and player-condition data and are refreshed when a player changes area.

Contribution access retains the client condition requiring completion of the
Broken Shore prerequisite quest `46286`, the Building state and at least 100 War
Supplies. The live database migration resets the three previously stuck building
states to a clean Building cycle. The DB2 relationships, SQL updates and Release
build have been validated, but an end-to-end client test of all three collector
buttons, rewards, state transitions and rotating buffs is still required.

## Class Hall, Legionfall and class mounts

The twelve Legion Class Hall campaigns lead into the level-110 Legionfall
campaign and its class-specific champion recruitment routes. The fifteen quests
required by **Breaching the Tomb** (`11546`) are present, including
**Champions of Legionfall** and its class campaign requirement. A level 100
boosted character can begin the Legion artifact and Class Hall progression, but
must reach level 110 and complete the Class Hall and Legionfall requirements
before starting its class-mount chain.

All twelve class-mount breadcrumbs now use **Breaching the Tomb** rather than
the unrelated **Broken Isles Pathfinder, Part One** achievement. Their complete
quest chains, quest-giver relations and final rewards are present. Death Knight,
Hunter, Monk, Paladin, Rogue, Shaman and Warrior receive their retail mount
items; Demon Hunter, Mage, Priest and Warlock learn their mounts from the final
reward spell. The Druid Lunarwing form is also learned immediately from its
final reward, with the existing login reconciliation retained for characters
that completed the quest before this correction. The formerly unguarded final
Warlock quest now requires its preceding quest and the Warlock class.

The class-specific scenario data and stage triggers were also audited against
the client criteria. Hunter's final feast stage is completed by its gossip
spell rather than a separate SmartAI credit. In the Death Knight **Lost
Glacier** scenario, collecting 100 **Essence of the Red** now completes the
third stage and exposes the client-defined **Life to Death** extra-action
button; using it advances the conversion stage, and turning in the final quest
casts the resurrection scene and completes the last stage.

The Paladin scenario currently has one functional set of six ally choices. The
client also defines six alternate-faction allies, but their creature templates,
spawn positions and faction phasing are not present in the world database.
Those NPCs must not be added as overlapping unconditional spawns; completing
that presentation requires reliable 7.3.5 spawn and phase data.

These relationships have been checked against the 7.3.5.26972 achievement,
criteria, quest, item-effect and mount data, and the canonical Release build has
been validated. Every class-specific scenario and the complete in-game hand-off
from Class Hall to Legionfall and the mount reward still require gameplay tests
before the twelve routes should be described as fully verified.

## Requirements

### Server and build dependencies

- Git.
- CMake 3.18 or newer.
- A 64-bit C++ toolchain. Visual Studio 2022 is the maintained Windows preset;
  the CI also exercises GCC 11 and Clang 14 on Ubuntu.
- Boost 1.78 or newer on Windows, or 1.74 or newer on other platforms.
- OpenSSL development libraries (OpenSSL 3.x is used by the Windows CI).
- MySQL-compatible client headers/libraries and a MySQL or MariaDB server.
- A legally obtained 64-bit World of Warcraft 7.3.5.26972 client.

The client, copyrighted game assets, extracted client data and preconfigured
local server credentials are not distributed by this repository.

## Building

### Windows x64

Install Visual Studio 2022 with the **Desktop development with C++** workload,
CMake support, Boost, OpenSSL and a MySQL-compatible client SDK. Then run:

```powershell
git clone https://github.com/ingussuveiks-dev/LegionCore-7.3.5.git
cd LegionCore-7.3.5
.\build.cmd
```

The script configures and builds the `default` CMake preset in Release mode.
Use `build-extractors/bin/Release` as the final runtime directory. The install
step also copies a package to `compiles`.

The equivalent explicit commands are:

```powershell
cmake --preset default
cmake --build --preset default
cmake --install build-extractors --config Release
```

Set `BOOST_ROOT`, `OPENSSL_ROOT_DIR` or `MYSQL_ROOT_DIR` when dependencies are
installed outside the locations detected by `cmake/WindowsDefaults.cmake`.

### Linux

Linux builds are covered by the GCC and Clang workflows in `.github/workflows`.
Install the equivalent compiler, CMake, Boost, OpenSSL and MySQL/MariaDB
development packages, then configure an out-of-source build. The exact package
names depend on the distribution.

## Databases

Four databases are used:

| Role | Base file |
| --- | --- |
| Authentication | `sql/base/auth_database.sql` |
| Characters | `sql/base/characters_database.sql` |
| World | `sql/base/LegionCore_full_735.26972_2024_10_23/LegionCore_world_735.26972_2024_10_23.sql` |
| Hotfixes | `sql/base/LegionCore_full_735.26972_2024_10_23/LegionCore_hotfixes_735.26972_2024_10_23.sql` |

Import the base schemas, copy the `.conf.dist` files to `.conf`, and configure
the four connection strings. Set `SourceDirectory` to the repository root if
the built-in updater should discover `sql/updates`. Review
`Updates.EnableDatabases` and `Updates.AutoSetup` before the first start, and
back up existing databases before applying updates.

Never commit real database passwords, private certificates or production
account data.

## Client data

Use the **26972** client and run the tools in this order:

1. `mapextractor`
2. `vmap4extractor`
3. `vmap4assembler`
4. `mmaps_generator`

The runtime data directory must contain the generated `dbc`, `maps`, `vmaps`,
`mmaps`, `cameras` and `gt` directories. Data extracted from another client
build is not a supported substitute.

## Configuration and startup

1. Copy `bnetserver.conf.dist` to `bnetserver.conf` and
   `worldserver.conf.dist` to `worldserver.conf`.
2. Configure database connections, `DataDir`, network bindings, certificates
   and `SourceDirectory`.
3. Start `bnetserver` first.
4. Start `worldserver` after the authentication service is ready.
5. Stop `worldserver` with `Ctrl+C` or the server shutdown command so pending
   database work and logs can be closed cleanly.

The executables search for their configuration beside the executable when no
explicit `-c` path is supplied.

## Known limitations

- Content coverage is uneven and inherited scripts/data include incomplete or
  unverified quests, encounters and class mechanics.
- Static DB2, SQL and source audits reduce invalid references but cannot replace
  in-game regression tests.
- Optional systems such as Warden, AHBot and Solocraft are not enabled by
  default and are not guaranteed to be correctly tuned for a public realm.
- Custom shop and boost flows require the matching current world database,
  current server binaries and the exact 7.3.5.26972 client.
- macOS is not currently covered by this fork's CI.

When reporting a bug, include the current commit hash, client build, database
update state, relevant `Server.log`/`DBErrors.log` lines, reproduction steps and
whether the problem also exists in the upstream project.

## Contributing

Keep fixes focused and reproducible. C++ changes should include a successful
build; SQL changes should be idempotent where practical and must preserve user
data. Gameplay fixes should cite the 7.3.5 client data or a reliable Legion-era
reference and document what was tested in game.

For inherited problems, review the
[upstream issue tracker](https://github.com/The-Legion-Preservation-Project/LegionCore-7.3.5/issues)
before opening a duplicate report.

## Credits and license

- [TrinityCore authors](https://github.com/TrinityCore/TrinityCore/blob/master/AUTHORS)
- [LegionCore contributors](https://github.com/dufernst/LegionCore-7.3.5/graphs/contributors)
- [The Legion Preservation Project](https://github.com/The-Legion-Preservation-Project/LegionCore-7.3.5)
- [LrdPsychoChains WOW HUB / LegionBotAI](https://github.com/psychochaingang/LrdPsychoChains-WOW-HUB/tree/main/projects/legionbotai)

See [COPYING](COPYING) for the core license text. The imported LegionBotAI
source is marked GPLv3 by its authors; see their
[LICENSE](https://github.com/psychochaingang/LrdPsychoChains-WOW-HUB/blob/main/LICENSE).
