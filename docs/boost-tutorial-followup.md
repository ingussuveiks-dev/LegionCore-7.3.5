# Boost tutorial: video and local data follow-up

Research only. No runtime/server/character state changed. Database queries
are SELECT-only and target `legion_world` and `legion_hotfixes` on the runtime
configuration's MariaDB endpoint. Configuration credentials are not exported.

## Reproduce the additional evidence

```powershell
& tools/research/Read-BoostServerEvidence.ps1 -OutputPath docs/boost-tutorial-server-evidence.json
& tools/research/Read-BoostSpellEffects.ps1 -OutputPath docs/boost-tutorial-spell-evidence.json
```

The server export contains three menus/options, their NPC text links,40 selected
broadcast records, relevant template bindings and negative binding checks.
The spell export contains18 effects for nine requested spell IDs. Its reader
checks the7.3.5 layout hash, section sizes and parent record indexes, supports
the selected fields' immediate/pallet-array/common storage, and exports raw
unsigned scalar values. It is not a full generic DB2 reader or spell resolver.

## Directly inspected gameplay evidence

Browser inspection of selected paused frames, not continuous playback:

- [Wowhead Warlock video](https://www.youtube.com/watch?v=1cCjo_KWxPo):
  00:05 visibly identifies a beta realm; do not label this recording7.3.5.
  00:28/00:46 identify Warlord Zakgra as instructor. Around05:27 the prior
  Horde Grunt has yielded; the next stage requests two attackers. Around07:01
  the demon-defense bar is45%; around07:38 it is84%. Exact yield-health
  threshold and wave spawn times cannot be inferred from these frames.
- [Wowhead Paladin video](https://www.youtube.com/watch?v=Nq0hhig9Q04):
  around06:09 General Bret Hughes has congratulated the player and the exit
  stage points to the gryphon at the back of the ship, while a demon remains
  alive. Around06:44 Armored Snowy Gryphon is selected and the Broken Shore
  battle quest is already accepted. The subsequent map transfer was not
  verified from this sampled frame.

No full dialogue transcript was copied from the videos. The DB text matches
below use the user's existing local data, not a transcription of website text.

## Gossip is partially present, execution bindings are not

| Actor | Entry | Menu | NPC text | NPC-text broadcast | Option broadcast |
| --- | --- | --- | --- | --- | --- |
| Warlord Zakgra | 105317 | 19768 | 29277 | 111964 | 117010 |
| General Bret Hughes | 100448 | 19768 | 29277 | 111964 | 117010 |
| Armored Snowy Gryphon | 107543 | 20085 | 29837 | 118580 | 118578 |
| Armored Wind Rider | 112565 | 20459 | 30357 | 121942 | 118578 |

Menu19768's option starts training; menus20085/20459 offer departure for Broken
Shore. The option-text fallback differs slightly from its broadcast text.
All referenced broadcasts exist in `legion_hotfixes` with VerifiedBuild26124.
Broadcast54564 matches the yielded opponent's line in the video;118216 matches
the female instructor's Affliction introduction. Nearby selected Warlock
broadcasts cover summon, Life Tap, Fear and spell-practice instructions.
Text presence does not establish exact scheduling or creature_text group IDs.

No matching SmartAI rows, menu conditions, or explicit spell_target_position
for219912 were returned by the scoped queries. Selected creature templates
have empty AIName/ScriptName. Entry114966 was found in the WDB name table
previously, but is absent from this creature_template binding query.

## Exact progression data for Warlock1084

Verified against the existing DB2 export and the core's achievement evaluator:

- First sparring stage: tree49427 requires one event48772.
- Second sparring stage: tree49432 requires two event48772 credits.
- Demon defense: tree49415 uses operator9 with threshold100. The core sums
  each child counter multiplied by its Amount. Felfire Imp102658 contributes5,
  Felblaze Infernal112639 contributes34, Ambushing Fel Bat111776 contributes17.
- Therefore the defense stage is a weighted progress threshold, not a
  requirement to kill every spawned demon. Video observation is consistent.
- The yield event must be attributed once to the correct player's current
  stage; precisely how stage transitions reset historical credits remains to
  be checked before implementation.

## Exit spell chain recovered from local DB2

Spell219912 is not a direct coordinate teleport. Its six effects are type140
(`SPELL_EFFECT_FORCE_CAST`) targeting96 (`TARGET_UNIT_PASSENGER_0`):

| Index | Triggered spell | Name/role |
| --- | --- | --- |
| 0 | 227058 | Leave for Broken Shore Queue |
| 1 | 215387 | Polished Armor |
| 2 | 215598 | Holy Enchantment |
| 3 | 215607 | Well Fed |
| 4 | 230503 | Boost2.0 default-talent driver |
| 5 | 231511 | Boost-experience gryphon safety timer |

Spell names were checked through public Wowhead tooltip responses for the
six triggers;227058,215387,215598,230503,231511 were also read directly from
the local Spell.db2 main records.215607 was not found in that direct main-record
pass; its effects exist locally, and copy/hotfix name resolution remains to be
checked. Do not use present-day tooltip labels to infer7.3.5 effects.

219615 and223763 are dummy effects (type3), respectively targeting22/7 and5/0.
227058 also has a local dummy effect; its exact record ID is in the JSON.

### Existing-code contradiction worth investigating

`src/server/scripts/Scenario/BrokenIslands/broken_islands.cpp` defines
`spell_q42740` for227058: it joins LFG slot908. The registration is absent and
a comment says227058 does not exist in7.3.5 client data. That claim conflicts
with the currently installed Spell.db2 name row and SpellEffect.db2 effect.
This is evidence that the comment cannot be trusted for this data set, NOT
proof that the complete loaded runtime SpellInfo and bindings are valid.

The separate scene trigger `enterBrockenShores` also joins908. Do not simply
enable both paths: distinguish tutorial exit from the normal introduction
scene and test for duplicate queues. The actual introduction instance script
is `instance_broken_islands` on map1460. The similarly named
`instance_AssaultBrokenShore` is map1666 and is not interchangeable with it.
The live `legion_world.instance_template` row for1460 is bound to
`instance_broken_islands`. A scoped live query returned no spell_script_names
bindings for219912,227058,230503 or231511. This confirms a missing binding,
not that adding it alone would finish the tutorial.

## Still unresolved

- Exact transport-local coordinates, boarding/vehicle seats, paths and spawn
  phases for both factions.
- Definitive target-dummy and sparring variants, surrender threshold, reset
  behavior and all wave timings.
- Full registration/conditions for the exit vehicle and loaded227058 spell;
  quest grants, subsequent effects and safe handoff/resume behavior.
- Tutorial selection and all class/spec flows, reconnect and failure recovery.

No automatic relocation should be enabled until these are implemented and
tested on disposable characters. Existing research remains partial.
