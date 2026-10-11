# Highmountain: Pet Rocks (39391), 2026-10-11

## Findings and fix

All three native objectives, click spells, models, static spawns, starter and ender were checked. Shard (95273) and Gravel (95276) already granted their correct credits from 189686, **Rescuing**. Frag (95272) instead used event 73, ON_SPELLCLICK, with parameter 1 set to 189686. For this event that field is a cooldown minimum, not a spell ID. The actual core swaps the reversed 189686..0 range and randomly disables the shared event for up to 189686 ms. A second player could therefore receive no Frag credit immediately after another player's interaction.

Migration 09 changes only Frag's handler to event 8, SPELLHIT, for its actual click spell **189687, Recovering**, with zero cooldown and the original personal credit 95272. Credit now follows the successful spell hit. Shard and Gravel retain their existing spell-hit behavior. All three click spells are restricted to the player having incomplete 39391, using source-18 conditions whose target 0 is the clicker in `Unit::HandleSpellClick`.

Frag's fallen presentation is intentional and remains unchanged. [The original Legion quest recording/description](https://warcraft.blizzplanet.com/pet-rocks-highmountain-quests/) and [Wowhead's quest text](https://www.wowhead.com/quest=39391/pet-rocks) establish that Frag has died; this fix does not incorrectly resurrect it. No global despawn, changed model, invented spell, personal-player pet manipulation or change to the quest objectives is introduced.

## Version evidence

- TDB735 objective 279326: Gravel 95276, storage index 1, amount 1.
- TDB735 objective 279327: Frag 95272, storage index 2, amount 1.
- TDB735 objective 279328: Shard 95273, storage index 3, amount 1.
- Both click spells are native dummy effects targeted at the clicked unit (target 25). Cast flags 1 select the player as caster, so the existing SmartAI invoker credit reaches the interacting player.
- Native model 64366 is retained for Frag; 60098 for Shard and Gravel. Existing GUIDs 340572/340594/340591 and their positions are unchanged.
- Navarrogg 95421 remains both starter and ender. The parallel refuge prerequisites from migration 07 remain intact.

## Validation

`Test-HighmountainPetRocksEvents.ps1` compiles the actual production ON_SPELLCLICK and SPELLHIT cases plus `SmartScript::RecalcTimer`. It reproduces the legacy maximum 189686 ms block, then verifies immediate matching-spell actions for two players on each corrected event, rejecting wrong and missing spell input. It does not substitute a separately rewritten timer algorithm.

`Test-HighmountainPetRocksData.ps1` applies the migration twice to temporary tables and checks native objectives, all three spell/cast/event links, zero shared cooldown, quest-only conditions, native models, Frag's fallen flag, actual spawns, turn-in and unchanged unrelated SmartAI. `-Live` checks deployment. `Test-HighmountainPetRocksNative.py` verifies the archived/native data and evidence hashes.

This is a server-side completion-path repair. Client rescue/recovery animations, any returning-pet presentation and narrative dialogue still require a 7.3.5 playthrough. These checks do not claim a fully observed retail scene.

## Included escape follow-up

This deployment also includes migration 08 and commit 85533cf0: absent child 190982 is handled through the native 190967 effect-2 periodic hook. It now dispatches the authorized cave escape instead of trying a nonexistent spell; the parent credit/stun and 190983 presentation are retained. See the updated Rock Escape audit and compiled periodic-handler regression.
