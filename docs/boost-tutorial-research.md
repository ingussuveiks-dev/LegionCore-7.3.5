# Legion 7.3.5 boost tutorial: evidence and integration gate

Status: research completed and implemented on 2026-09-22. The raw evidence in
this document remains the provenance for the implementation in
`Scenario/BoostExperience/boost_experience.cpp` and world update 311.
Collected against the local extracted DB2 and MariaDB `legion_world`.

The implementation uses the client-defined class/spec scenario criteria,
maps 1554/1557, transport entries 246606/246607, paths 5528/5529 and
WorldSafeLocs 5219/5752. Server-side NPC placement and combat orchestration
are supplied by the instance script because neither public TDB snapshot
contains those missing transport passengers.

## Reproducible client evidence

Run from the repository root:

```powershell
& tools/research/Read-BoostScenario.ps1 -OutputPath docs/boost-tutorial-db2-evidence.json
```

The reader checks the four layout hashes against this core's DB2Metadata.h,
rejects unsupported storage, and checks selected step/tree/criterion references.
The JSON contains source SHA256 hashes, 14 Combat Training scenarios, 186 steps,
536 criteria-tree records and 116 criteria. This is the complete descendant
closure of those selected step roots, not all data needed for gameplay.
Numbers are raw unsigned storage values; signed fields are not sign-extended.
No selected criterion references a ModifierTree.

Class descriptions associate scenarios as follows; specialization eligibility
must still be verified separately (do not choose solely by class):

| Class | Scenario IDs |
| --- | --- |
| Paladin | 1073 |
| Hunter | 1083, 1214 |
| Warlock | 1084 |
| Priest | 1090 |
| Shaman | 1091, 1182 |
| Warrior | 1093 |
| Death Knight | 1094 |
| Monk | 1095 |
| Druid | 1096, 1181 |
| Mage | 1132 |
| Rogue | 1133 |

### Warlock 1084

| Order | Title | Criterion assets (not criterion IDs) |
| --- | --- | --- |
| 0 | Combat Training | Cast 219615 |
| 1 | Demonic Bidding | Cast 697 / 223763; preserve criteria-tree operators |
| 2 | Curses! | Cast 980 |
| 3 | Corrupting Power | Cast 172 |
| 4 | Stealing Health | Cast 198590 |
| 5 | Add Some Instability | Cast 30108 |
| 6 | Putting It All Together | 980, 172, 198590, 30108; preserve tree operators/counts |
| 7 | Life Tap | Cast 1454 |
| 8 | Fear | Cast 5782 |
| 9 | We're Going To Need A Bigger Demon | Cast 18540 |
| 10 | A Quick Test | Script event 48772 |
| 11 | Challenge Accepted | Script event 48772 |
| 12 | Defend Against the Legion Attack | Kill 112639 / 102658 / 111776; preserve tree operators/counts |
| 13 | Assault on Broken Shore | Be target of spell 219912 |

Type interpretation comes from `src/server/game/DataStores/DBCEnums.h`:
0 = KILL_CREATURE, 28 = BE_SPELL_TARGET, 29 = CAST_SPELL,
92 = SCRIPT_EVENT_2. Full tree structure/counts remain in the JSON.

Local `creature_template_wdb` confirms 112639 Felblaze Infernal,
102658 Felfire Imp and 111776 Ambushing Fel Bat. Their creature-template
ScriptName fields are empty. There are no spell_script_names bindings for
219615, 223763 or 219912. These negative checks do not rule out every other
possible implementation mechanism.

## Transport evidence

| Faction | Scenario map | Existing local GO template | Taxi path | Transport interior map |
| --- | --- | --- | --- | --- |
| Alliance | 1554 | 246606 Dawn Blade | 5528 | 1560 |
| Horde | 1557 | 246607 The Warbringer | 5529 | 1559 |

The local TaxiPathNode records put 5528 on map1554 and 5529 on map1557.
WorldSafeLocs 5219 (Alliance) and 5752 (Horde) have near-origin coordinates
consistent with transport-local positions. Their coordinate space MUST be
verified before use; do not treat these as safe world-space teleport targets.

No creature/gameobject spawns were found on maps1554/1557/1559/1560 and no
instance_template rows on1554/1557. A GO template alone is not a running tutorial.

## Upstream review

Blizzard describes a gunship tutorial followed by Broken Shore:
[Getting a Boost to Level 100](https://worldofwarcraft.blizzard.com/en-us/news/20230441).

Reviewed partial implementation:
[frostwolfcore/Legion7.3.5](https://github.com/frostwolfcore/Legion7.3.5),
commit `c31d9360f0af0e0ba72287185dd263738feecd9b`, also found in
Psychostout/merged-reforged_LegionCore-7.3.5. Reference checkout is separate
from the working repository. No upstream code was imported.

Its `sql/updates/world/2026_02_26_boost_experience_transports.sql` inserts
GO204018/204423. Their local taxi paths2222/2337 belong to maps646/654, not
the boost maps. Reject this SQL as-is. Its ClassTrialHandler mixes apparent
world and transport-local positions and uses class-trial lifecycle rules;
it is not a verified paid-boost implementation.

Repository-tree/name inspection also covered Titans-Project/LegionCore-Reforged,
The-Legion-Preservation-Project/LegionCore-7.3.5, AshamaneProject/AshamaneCore,
dufernst/LegionCore-7.3.5, fluxurion/Legioncore_uwow and
psychochaingang/LrdPsychoChains-WOW-HUB. This was not an exhaustive audit of all
SQL/content in those projects and does not prove no working implementation exists.

## Missing data: do not claim completeness

### Additional web evidence (2026-09-22)

Primary Blizzard sources confirm the intended player-facing flow:

- [Getting a Boost to Level 100](https://worldofwarcraft.blizzard.com/en-gb/news/20230441/getting-a-boost-to-level-100): a preset tutorial specialization, gunship
  practice, an optional flight-based skip, then Broken Shore. This does not
  establish the underlying spell/NPC IDs or world coordinates.
- [Learn About Legion Class Trials](https://worldofwarcraft.blizzard.com/en-us/news/20245594/learn-about-legion-class-trials), October 6, 2016: trial characters
  start at100 with a preset spec and can proceed to acquiring their artifact.
  Keep trial locking separate from already-paid boost entitlements.

[Wowhead's own Legion class-trial guide](https://www.wowhead.com/guide/new-players/class-trials)
contains embedded gameplay videos. Its Warlock recording is
[Legion Class Trial - Warlock](https://www.youtube.com/watch?v=1cCjo_KWxPo),
author Wowhead (title/author verified through YouTube oEmbed).
Selected video frames have now been inspected (see the follow-up below), but
not a continuous timeline or precise spawn coordinates. The guide's older Warlock
ability list includes689 whereas our DB2 step uses198590: local7.3.5 data must
take precedence over an older recorded build.

Other embedded recordings, suitable for follow-up visual verification:

| Class | YouTube video ID |
| --- | --- |
| Death Knight | CIWTbmyGHJo |
| Druid | ybzEeHdXeDw, 97IXtTXvIPM |
| Hunter | 4ZGWqMppiy4 |
| Mage | htmBDte7XFg |
| Monk | WZFJ43moMlA |
| Paladin | Nq0hhig9Q04 |
| Priest | ml5wE0q5AJw |
| Rogue | 2rcp7g0YPEk |
| Shaman | gFNrQpRRh7o |
| Warrior | EikT2gl1Q7E |

Wowhead live tooltip endpoints returned these names (current labels, not a
version-pinned7.3.5 effects dump):

| Spell | Reported name | Interpretation limit |
| --- | --- | --- |
| [219615](https://nether.wowhead.com/tooltip/spell/219615) | Boost 2.0 [All] - On Talk to Paragon | Supports a trainer-conversation trigger; not its gossip implementation |
| [223763](https://nether.wowhead.com/tooltip/spell/223763) | Boost 2.0 [All] - Check For Pet | Supports checking an existing pet; not exact aura/effect logic |
| [219912](https://nether.wowhead.com/tooltip/spell/219912) | [DND] Leave | Supports an exit action; does not provide destination or prerequisite handling |

Secondary discovery leads: the Legion-specific NPC lists on
[Tempest's Roar](https://warcraft.wiki.gg/wiki/Tempest%27s_Roar) and
[Sword of Dawn](https://warcraft.wiki.gg/wiki/Sword_of_Dawn).
These pages also discuss later expansions. Only their Broken Shore sections
were used to select names for a read-only lookup in local creature_template_wdb.
The resulting IDs below verify local template existence, NOT scenario placement:

| Faction | Candidate name | Local entry IDs |
| --- | --- | --- |
| Horde | Warlord Zakgra | 105317 |
| Horde | Sergeant Ba'sha | 112026 (older namesake12799 excluded) |
| Horde | Staff Sereant Lazgar (local spelling) | 112025 (older namesake48062 excluded) |
| Horde | Cain Firesong | 112038 (older namesake2128 excluded) |
| Horde | Seer Janidi | 112002 (older namesake18017 excluded) |
| Horde | Armored Wind Rider | 112565 |
| Alliance | Admiral Walsh | 95093 |
| Alliance | General Bret Hughes | 100448,114966; selection unresolved |
| Alliance | Magus Filinthus | 105628 (older namesake16821 excluded) |
| Alliance | Anchorite Taliah | 107953 |
| Alliance | Lieutenant Warren / Murray | 107541 /108988 |
| Alliance | Navigator Lane / Gibson | 108983 /108982 |
| Alliance | Armored Snowy Gryphon | 107543 (older namesake32335 excluded) |
| Both/unknown | Arcane Construct | 101738,108478; selection unresolved (older36599 excluded) |

[Wowhead NPC105317](https://www.wowhead.com/npc=105317/warlord-zakgra)
independently associates Zakgra with Broken Shore as well as Seething Shore;
the mixed locations reinforce the need to separate expansion versions.
No creature_text rows were returned for the six Horde candidate entries above.

Searches of2016 Blizzard hotfix material did not yield the missing tutorial
spawn coordinates, wave timing or scripts. This is a limited negative result,
not proof such information was never published. Several direct pages were
unavailable through the web reader; indexed text and public tooltip responses
were distinguished from directly inspected video/gameplay.

### Remaining integration blockers

Follow-up evidence is recorded in `boost-tutorial-followup.md`; it resolves
some player-facing behavior, gossip mappings and exit spell links below, but
does not remove the full integration gate.

- Verified trainer, target-dummy and sparring-opponent entries, transport-local
  positions, factions, phase conditions, gossip and dialogue sequencing.
- Combat/spawn-wave timing and the exact trigger/reset rules for event48772.
- Initiation and exit spell behavior, including effects, scenes and destinations.
- Scenario selection by class/spec/faction and supported boost-vs-trial rules.
- Instance creation, transport boarding, disconnect/reconnect persistence,
  skip path and safe recovery if the tutorial fails.
- Verified handoff into each faction's Broken Shore chain, including quest,
  phase and scenario prerequisites; no assumption that this chain already works.

A matching historical packet capture or complete compatible server scripts/SQL
would resolve the server-only gaps. DB2 alone cannot provide them. A deliberately
custom reconstruction is a separate decision, not authentic data collection.

## Integration/test gate

1. Resolve the above missing-data items with provenance, validate referenced
   IDs against local DB2 and `legion_world`, review upstream licensing.
2. Prepare scoped migration and server scripts with recovery/skip handling.
3. Test on disposable Alliance and Horde characters: every stage, every supported
   class/spec, skip, death, logout/reconnect, restart and final handoff.
4. Verify repeated paid boosts remain account entitlements, not class-trial locks.
5. Only then enable automatic relocation and move the user's requested character.

Any completed Release executable belongs in `build-extractors/bin/Release`;
preserve runtime configs, certificates, DLLs and extracted data.
