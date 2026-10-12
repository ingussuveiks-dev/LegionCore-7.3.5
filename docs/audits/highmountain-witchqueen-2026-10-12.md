# The Witchqueen (39322)

High Crawliac's kill objective and handoff were intact. Her combat configuration skipped the native cyclone creation chain and allowed generic AI to choose self for explicit-target offensive spells. This batch repairs those combat paths without changing quest objectives or granting synthetic kill credit.

## Evidence and changes

- TDB735 and runtime 7.3.5 DB2 evidence is recorded in `highmountain-witchqueen-native-2026-10-12.json`. Objective 279206 requires one kill of 95153, storage 0, flags 1. Lasan 95410 ends the quest; RewardNextQuest is 39387. Both Haglands rewards remain required through the preceding batch's negative exclusive group.
- Live spawn 282952 is unphased on map 1220, area 7728, at 5079.16 / 4411.78 / 1043.14. Its faction is hostile and scaling is 98..110. Lasan spawn 340584 and zone phase entry 25 provide the nearby turn-in. These records were checked and retained.
- The previous default AggressorAI consumed `creature_template_spell`, including damage child 192184 and older Brilliant Edge damage 166312. `UnitAI::FillAISpellInfo` examines TargetA: target 25 (`TARGET_UNIT_TARGET_ANY`, default check, target reference) falls through to SELF, and `UnitAI::DoCast` uses that classification. Native 192287 has the same explicit target. A scoped SmartAI configuration now casts Vicious Storm 192173 on self, Shredding Cyclone **parent 192177** on self, and A Feast of Eyes 192287 on the current victim. The catalog rows are retained but no longer automatically scheduled by this NPC's AI. No global spell or default-AI behavior changed.
- Native dummy effects 0..4 of 192177 encode children 192179..192183. Five option-6 dummy links, each with its own effect bit and HIT_TARGET mask, cast the native children without substituting spell IDs for damage. These were missing entirely. The children create area triggers 4889 and 2736; native duration is 5000 ms.
- Existing area-trigger templates already contain movement, radius and visual 49781 but had no contact actions. Added contact damage 192184 with VALIDATTACK and SCRIPT gates. Native 192184 damage is retained. Script bindings use custom entries 9605 and 7345, as required by this core's AI lookup. The gate requires a living, fighting High Crawliac, one of the five native cyclone spells, and a living attackable target. Shared Brilliant Edge templates cannot execute this new action. Cyclones despawn when their caster dies, leaves combat or disappears; normal expiry remains native.
- Combat initial delays 4/8/12 seconds and repeats 15/18/12 seconds are server tuning, not proven retail timings. No unproven summon behavior for 192333 was added.

Contemporary primary quest capture confirms the kill and Lasan handoff: [Blizzplanet, 2015-12-14](https://warcraft.blizzplanet.com/the-witchqueen-highmountain-quests-alliance/). Version-specific IDs and effects above come from local 7.3.5 data, not the current website database.

## Validation

- `Test-HighmountainWitchqueenNative.py`: TDB objective/ender/next quest; actual DB2 five dummy children, AT effects, finite lifetime, damage and aura targets; reproducible evidence hashes.
- `Test-HighmountainWitchqueenData.ps1`: temporary-table migration applied twice, exact targets/links/contact flags/bindings, reachable actors and scaling, existing shared templates and unrelated rows preserved. `-Live` checks deployed records.
- `Test-HighmountainWitchqueen.ps1`: compiles the actual production AT script. Tests every cyclone child, self/friendly/dead/null rejection, other-caster/older-spell exclusion, and death/evade/lost-caster cleanup.
- Expanded `Test-HighmountainTheThree.ps1`: extracted production dummy handler emits exactly five distinct children from five effects, not 25, without custom damage overrides.
- Expanded `Test-HighmountainBloodtotem.ps1`: extracted production kill/completion/prerequisite handlers require High Crawliac's death, cap duplicate credit, ignore unrelated/absent/rewarded kills, and require reward before 39387.
- Release scripts compilation passes. Deployment uses the canonical `build-extractors/bin/Release` executable with database/runtime backups and protected-file checks.

These are source, data and extracted-handler checks, not a client playthrough. Still check the visible five moving cyclones and collision damage, Feast targeting, storm knockback, evade/re-pull, queen kill and Lasan reward in the 7.3.5 client. Retail timing/balance and additional scripted choreography are not claimed verified. The custom level-100 boost, ship, scenario and pet paths were not edited.

Next batch: **39387, The Skies of Highmountain**, including its talk/flight/arrival objectives and delivery back to Skyhorn.
