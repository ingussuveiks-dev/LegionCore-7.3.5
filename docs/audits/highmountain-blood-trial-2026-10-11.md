# Cave of the Blood Trial (39455)

## Findings

The altar's existing spell 192443 is the native Summon Warmonger Challenger:
SpellEffect 282310 summons creature 97544, using wild SummonProperties 3640,
destination target 142 and a 120-second duration. The live challenger had no
trial script, immunity to NPCs (including player pets), and Classic health
scaling expansion 0 instead of native Legion expansion 6. Generic altar use
credited the first objective and used shared goober state. There was no personal
duel ownership, nonlethal victory or retry handling.

The native objectives already match TDB735 build 25549: altar 244564 in slot 0,
then Warmonger 97544 in slot 1 (Flags=3: tracked and sequenced). Neither objective
is optional. They remain unchanged. Existing Torok 93836 is the ender, offers
39860 after reward and retains its acceptance spell 190902.

## Changes

The altar now handles each player separately. It validates the active quest,
spawned/visible object, phase, distance and line of sight. It creates a private
Witch presentation using existing entry 97539/model 64303 at the altar origin,
then casts the existing native challenger summon at the altar. The first
objective is recorded only after a challenger exists. Failed creation rolls
back the new actors without granting credit. Repeated clicks during a fight
do not duplicate summons. Native AllowMultiInteract=1 is restored.

The Witch entry/model are confirmed in TDB735; the corpse presentation uses the
live template's existing feign-death flag. Its placement is reconstructed from
the actual altar position, not asserted to be a sniffed original corpse offset.
No additional harpy kill or invented inventory item is required.

Warmonger is visible to the owner, attacks only that player or their controlled
units, and ignores damage from other players. The existing AggressorAI still
handles melee and the existing Burning Slash (195789); assistance/fleeing is
disabled for this personal trial. Only the private actor loses combat-blocking
flags. Its health expansion is corrected to 6 while preserving the valid
98..110 level-scaling range.

A would-be lethal hit becomes surrender before death, using the same damage
scaling division and target-relative health as the core's death check. This also
handles a fresh opponent hit for a billion points. No unverified retail surrender
percentage is introduced. Surrender stops attacks/casting, prevents later damage,
removes the caster's Burning Slash from the owner and finishing attacker, credits
only the owner, and sends the existing Torok response privately. The actor then
despawns. Generic group kill rewards are not used for this scripted victory.

The owner can click the altar again after death, relog, map change, evade, range
loss or summon timeout. The first objective persists; the fight restarts without
awarding the second objective. Quest lifetime guards invalidate old actors after
abandon/reaccept. Cancelled fights remove their own Witch presentation, not a
new attempt's actors. No completed/rewarded saves are reset.

The installed BroadcastText.db2 is partial; lines 99485/99570 are present in the
hotfix database. The unreadable challenger `creature_text` fallback is restored
from the existing hotfix 99485 while retaining that BroadcastTextID.

## Verification

```powershell
python tools/tests/Test-HighmountainBloodTrialNative.py
tools/tests/Test-HighmountainBloodTrial.ps1
tools/tests/Test-HighmountainBloodTrialData.ps1
tools/tests/Test-HighmountainBloodTrialData.ps1 -Live
```

Native evidence, source hashes, spell effects, summon properties, duration,
creature models and objectives are stored in
`highmountain-blood-trial-native-2026-10-11.json`. Properties have Control=0,
Title=0 and Slot=0: this is not a pet/guardian-slot replacement.

The C++ harness compiles extracted production altar, challenger and quest-lifetime
handlers. It covers two concurrent users, ordered credit, duplicate clicks,
failed body/challenger summons, invisible/remote/occluded altar, other-player and
pet damage, normal hits, both scaling directions and billion-damage surrender,
no repeated victory, existing combat-AI forwarding, death/relog, abandon/reaccept,
evade, map, phase and distance recovery. SQL tests apply the migration twice in
temporary tables and verify native objectives, script bindings, valid scaling,
combat spell, visual, actual spawns, Torok handoff and unrelated data.

The actual installed ground navmesh connects altar spawn 112983 at
5307.56,4997.42,627.498 to Torok spawn 283367 at 5293.96,4965.95,626.776 through
six polygons. This is connectivity evidence, not client visual verification.

The firsthand [Blizzplanet quest recording](https://warcraft.blizzplanet.com/blog/comments/cave-of-the-blood-trial-highmountain-quests)
supports altar placement, challenge, victory and Torok's response. Its alpha-era
presentation supplements the version-specific native data, rather than defining
combat tuning.

## Client retest still required

Follow Oakin into the cave, use the altar, check corpse placement and private
visibility, fight with and without a pet, use a one-hit lethal spell, then turn
in at Torok and accept 39860. Repeat with two players and after death/relog or
abandon/reaccept. Confirm localized dialogue, Burning Slash visuals and corpse
pose in the 7.3.5 client. These client checks are not claimed as completed.

The next separate audit is 39860, Rite of Blood. Its actual ritual/final challenge
is not verified by checking its prerequisite and acceptance here. Boost, ship,
pet lookup, camera and global damage/scenario code remain unchanged.
