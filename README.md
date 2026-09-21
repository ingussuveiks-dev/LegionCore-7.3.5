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

Warden requires compatible checks and careful validation before use. AHBot also
needs an account/character setup and deliberate economy configuration; changing
only one switch is not a production economy setup.

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
Build products are created under `build-extractors/bin/Release`; the install
step copies the runnable package to `compiles`.

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

This project is distributed under the GNU GPL v2. See [COPYING](COPYING).
