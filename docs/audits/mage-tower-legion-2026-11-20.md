# Legion Mage Tower audit · build 26972

The seven Legion challenges are present. All 36 specializations have the correct
scenario and base appearance defined, and the 20 scenario stages have their
required progression producers. The audit found encounter mechanics, admission,
pet credit and tint validation that need work before the tower can be described
as complete.

## Warlock

| Specialization | Encounter | Server findings |
| --- | --- | --- |
| Affliction | Thwarting the Twins · 1273 / 1684 | Karam `116410` has both Purgatory transitions (`235308`), at 40% and 20% in the live data. The orbiters summon the shadow adds. Karam directly summons Hand from Beyond `118698`, which casts Grasp from Beyond `235578`. Raest's death produces `56198`. The later Rune of Summoning sequence (`236469` → `236705` → `236460`) has no reachable cast in the checked encounter graph. Thing of Nightmare `119098` has a client creature record and lacks a world `creature_template` row. [Original encounter reference](https://www.wowhead.com/guide/affliction-warlock-mage-tower-challenge) |
| Demonology | The God-Queen's Fury · 1265 / 1703 | EventObject `536` starts the scenario. Sigryn, Runeseer Faljar and Jarl Velbrand share health through `239056`. Sigryn has Blood of the Father `237945` and a surrender branch that produces `56052`. Runic Detonation is present. Dark Wings `237730` and Ancestral Knowledge `237949` have no reachable cast in the checked graph. Dark Val'kyr `119696` lacks a world creature template. The surrender and shared-health behavior on a lethal hit still need a controlled combat check. [Original encounter reference](https://www.wowhead.com/guide/demonology-warlock-mage-tower-challenge) |
| Destruction | Feltotem's Fall · 1266 / 1702 | Tugar `117230` casts Fel Burst `242733` and Fel Shock `242730`. Jormog `117484` has Fel Hardened Scales `238471`, Sonic Scream `241687` and the death-linked Fel Rage path. The Earthquake and Fel Surge Totem branches (`237950`, `241346`, `241355`, `243224`, `237903`) were absent from the checked cast graph. Fel Surge Totem `121499` lacks a world creature template. Removing scales through charging fallen rocks therefore remains unestablished. The pet credit defect below also applies to Warlock demons. [Original encounter reference](https://www.wowhead.com/guide/destruction-warlock-mage-tower-challenge) |

## Warrior

| Specialization | Encounter | Server findings |
| --- | --- | --- |
| Arms | Closing the Eye · 1204 / 1673 | Xylem `115244` has Frostbolt, Razor Ice, Comet Storm, the Arcane transition, Draw Power and the 10% surrender. Corrupting Shadows `116839` is summoned by the outro and has its own death producer `55528`. The checked script switches Frost to Arcane by health. The original Mirror Image intermission and its safe bubbles still require reconstruction evidence. Both Victoria choices use `229196`. The paid choice requires 100 Nethershards in gossip, and `229196` has no currency reagent in the installed DB2 or the copied hotfix store. [Original encounter reference](https://www.wowhead.com/guide/arms-warrior-mage-tower-challenge) |
| Fury | An Impossible Foe · 1221 / 1616 | Agatha `115638` has Shadow Stab, imp summons, Dark Fury, Agatha's Vengeance and death event `55521`. The local row casts Agatha's Fury `242879` periodically during combat. The Legion reference describes the effect after imp deaths and arena changes tied to health. The checked Agatha graph has no 50% arena transition. Keep this challenge distinct from the Fire Mage acquisition scenario `1122` on the same map. [Original encounter reference](https://www.wowhead.com/guide/fury-warrior-mage-tower-challenge) |
| Protection | The Highlord's Return · 1259 / 1698 | Variss has Aura of Decay, Mind Rend, Drain Life and Tormenting Eye. Kruul has Netherstomp `234673`, Annihilate `236572` and the final sequence. Twisted Reflection `234676`/`234678` has no reachable cast in the checked graph. The Variss and Kruul death chains submit survival events `56553` and `56796` without checking whether Velen and Kor'vas are alive. An independent failure path for those allies still needs proof. [Original encounter reference](https://www.wowhead.com/guide/protection-warrior-mage-tower-challenge) |

## Hunter

| Specialization | Encounter | Server findings |
| --- | --- | --- |
| Beast Mastery | Feltotem's Fall · 1266 | Reproduced a pet/guardian final-blow problem. Tugar `117230` action `7` and Jormog `117484` actions `14`/`15` use SmartAI target `16`. `SmartAI::JustDied` forwards the actual killer. The target selector requires `trigger->ToPlayer()` and produces an empty list for a pet or guardian, before the credit code can resolve the owner. Both the final kill-credit spell `233870` and Jormog's scenario criterion `58088` can be lost. The extracted production selector reproduced this for both a solo owner and an owner with a group. The player-kill paths produced their expected target lists. |
| Marksmanship | Thwarting the Twins · 1273 | The specialization selects the Twins as expected. Raest's death criterion uses the closest-player selector, distinct from the target-16 defect. Rune soaking by the player or pet stays unverified because the checked graph lacks the native rune sequence. |
| Survival | Closing the Eye · 1204 | The specialization selects Xylem and Corrupting Shadows. The last criterion has a real death producer on the summoned boss. Mirror Image intermission, simultaneous deaths and the final pet/guardian blow remain client or runtime combat checks. |

The base artifact appearance IDs for the nine priority specializations, in
review order, are `972`, `828`, `915`, `806`, `665`, `861`, `466`, `965` and
`973`. Each matches the normal weapon's own appearance set and the hidden quest
associated with its challenge.

## Shared coverage for the remaining classes

Each challenge below uses the same encounter for the listed specializations.
The specialization gates and teleport destination were checked individually for
both Victoria choices per specialization.

| Challenge | All assigned specializations |
| --- | --- |
| Thwarting the Twins | Affliction Warlock, Marksmanship Hunter, Balance Druid, Frost Mage, Shadow Priest |
| The God-Queen's Fury | Demonology Warlock, Arcane Mage, Retribution Paladin, Assassination Rogue, Enhancement Shaman |
| Feltotem's Fall | Destruction Warlock, Beast Mastery Hunter, Discipline Priest, Windwalker Monk |
| Closing the Eye | Arms Warrior, Survival Hunter, Frost Death Knight, Havoc Demon Hunter, Subtlety Rogue |
| An Impossible Foe | Fury Warrior, Unholy Death Knight, Feral Druid, Fire Mage, Outlaw Rogue, Elemental Shaman |
| The Highlord's Return | Protection Warrior, Blood Death Knight, Vengeance Demon Hunter, Guardian Druid, Brewmaster Monk, Protection Paladin |
| End of the Risen Threat | Restoration Druid, Mistweaver Monk, Holy Paladin, Holy Priest, Restoration Shaman |

- **Discipline admission**. Feltotem quest `45627` has class mask `256` twice
  and omits Priest mask `16`. Its paid and free teleport actions already
  correctly include Discipline specialization `256`. Source-19 admission fails
  earlier, where `256` means the Warlock class bit. Preserve rewarded prerequisite
  `45842` when repairing the missing Priest branch.
- **Healer encounter**. The seven stages have actual source-13 EventObjects,
  door actions, heal-gated Damaged Soul credits and Erdris' death event. The
  death rows for Jarod `118447`, Granny Marl `118448` and Callie `118451` use
  Restart `236132`. The five native alternative branches carrying `56912`
  describe survival. The live graph has no producer that grants that event as
  completion credit. A client run must still exercise the timed rescue, eight
  heals, Risen Army and an actual ally death.
- **Victory and base appearance**. Victoria `117871` is bound to event `400`.
  The seven map entrances and specialization teleport rules match the client
  catalog. Each of the 36 base appearance conditions is directed by generated
  DB2 hotfixes to the corresponding quest `45902`, `45904`, `45905`, `45906`,
  `45908`, `45909` or `45910`. The seven quest-giver accept actions cast
  `234765`, whose removal triggers `229414` and the real appearance credit
  `115713`. The seven quest templates require one such credit. The final
  scenario steps have `RewardQuestID=0`; the encounter victory and the later
  hand-in/forge chain are separate milestones.
- **Tints**. All 108 secondary tint conditions use the original modifier trees
  plus tint achievement conditions. The snapshot has no rows overriding those
  conditions, and the generated core hotfixes apply to the 36 current base
  condition IDs. The modifier evaluator lacks the original quest-state branch
  types `84`, `110` and `174`, so the per-challenge quest checks used by the tint
  trees are not enforced as written. Test the actual tint state before and after
  earning the base appearance. Preserve the separate tint achievements.

## Evidence and limits

Compared the copied MariaDB world and hotfix data with the installed Legion
DB2 files, local source and the newer local TrinityCore reference. The checker
read 331 creature placements, 67 gameobjects, four EventObjects and 799 SmartAI
rows, including referenced timed lists and selected summoned actors. It checked
72 entrance spell selections, 20 native stage graphs, the 36 base models/quest
gates and 108 tint conditions. Spell coverage follows the checked creature
templates, passive and addon auras, SmartAI, native spell triggers and selected
linked-spell rows. Timing, distance, collision, interrupts and encounter
polish remain game checks for the exact 7.3.5 build. The references are written
Legion-version accounts. No video or new client playthrough was used.

The canonical executable started with loopback ports, disposable account and
character schemas, and the isolated MariaDB copy. It reached readiness and
exited gracefully. Server diagnostics and the pet selector probe produced no
new crashes, appended DBErrors or unexplained runtime errors. The first world
snapshot query was deliberately canceled after a slow nested sort. MariaDB
logged that cancellation. The indexed readback was repeated after restarting
the isolated database and produced the same hashes for the seven selected
exports. The second database process shut down with exit 0 and no new warning.

Local evidence is under `build-extractors/bin/Release/fixed/2026-11-20-mage-tower-audit`.
The 98-file manifest has SHA-256
`F05CEAFA3A8A411E1178B7B4AC883E13B4CAFCDD80144CF3D0652656E17DF758`.
It includes `routes.json`, the native stage and cast graphs, the pet repro and
diagnostics. The generated probe executable and object were removed. The
reused isolated database copy is stopped. The production SQL and executable
remain at the audited baseline.

Fresh admission, logout/re-entry, paid and free retry, death and recovery,
exit portal use, quest hand-in and forge verification each require a client or
combat run. In particular, Xylem's outro contains a portal producer for GO
`267033`. The first-free-attempt marker and its consumption should be checked
together with the paid option when repairing that route.

Original entry context and solo challenge design are documented in the
[Legion 7.2 preview](https://worldofwarcraft.blizzard.com/en-us/news/20602173/patch-7-2-preview-a-challenging-appearance).
The original specialization matrix is also recorded in the
[Legion artifact appearance guide](https://www.wowhead.com/guide/legion-mage-tower-class-challenge-weapon-appearances-4991).
