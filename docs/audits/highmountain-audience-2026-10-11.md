# An Audience with Torok (39374)

## Confirmed defects

Oakin's acceptance SmartAI only spoke text group 0; it never started an escort
to the tent. The destination Oakin used a 3-yard proximity trigger and timed
list 9525600. Its immediate kill-credit action targeted **all players within
10 yards**, before the guard's delayed reply. This allowed other players to
finish from someone else's trigger and bypassed the intended accompanied visit.

The native TDB735 objective remains unchanged: objective 279991, creature
95256, one count, storage slot 0. Native evidence and archive hash are in
`highmountain-bloodtotem-native-2026-10-11.json`; reproduce with
`Test-HighmountainBloodtotemNative.py`.

## Repair

Each active player receives a personally visible Oakin. He walks to the existing
destination spawn, waits when the player falls behind, and requires a matching
movement-completion callback at that destination. Both participants must be near
the living, visible Bloodtotem Warmonger 98904. Oakin speaks the existing question;
five seconds later the existing localized guard response is sent privately to
the owner. Only then is that player's objective updated. No area/group kill
credit or invented creature, spell, or conversation ID is used.

The shared acceptance/proximity rows, their conditions and old timed list are
removed. Oakin now derives from SmartAI, retaining normal SmartAI for static
spawns and the later cave summon. Audience summons deliberately skip the
JUST_SUMMONED cave path. The questgiver flag returns after completion. Reward
hands the personal actor back to SmartAI with a 60-second grace period to accept
39455; audience cleanup does not remove this handoff actor. The existing cave
acceptance list 9620856 and all seven cave waypoints remain unchanged.

Recovery runs only around the existing start/end area, for living, grounded
players without a vehicle. Logout, map change, death, flight, phase/distance loss,
abandonment or a timed-out path remove the active actor. Returning retries the
scene. Quest lifetime guards prevent an abandoned actor from crediting a newly
accepted copy, including abandonment/reacceptance between update ticks. Failed
spawns retry. Completed saves remain completed; prior erroneous credits cannot
be distinguished from legitimate ones, so they are not revoked. A completed
player reconnecting in the area receives a turn-in actor without repeated credit.

## Evidence and checks

Start (existing spawn 283321): 5221.07, 4902.25, 711.837.
Destination (existing spawn 340589): 5335.47, 4919.65, 691.969.
Guard (existing spawn 283352): 5338.3, 4922.95, 692.054.
The actual Release navigation reader connects the two Oakin positions through
**34 ground polygons** in the installed 7.3.5 map 1220 data. This checks navigable
connectivity, not a historically sniffed escort route or client animation.

The firsthand [Blizzplanet quest recording and transcript](https://warcraft.blizzplanet.com/blog/comments/an-audience-with-torok-highmountain-quests)
shows the accompanied visit, Oakin's question and the guard directing the player
to the cave. Its alpha-era timing is supporting context; the objective and IDs
are checked against the version-specific database and installed server data.

```powershell
tools/tests/Test-HighmountainAudience.ps1
tools/tests/Test-HighmountainAudienceGeometry.ps1
tools/tests/Test-HighmountainAudienceData.ps1
tools/tests/Test-HighmountainAudienceData.ps1 -Live
python tools/tests/Test-HighmountainBloodtotemNative.py
```

The script test compiles extracted production handlers and the core quest-lifetime
helper. Cases include two players, no spawn/proximity-only credit, wrong movement
callback/location, interrupted dialogue, missing/dead/invisible guard, owner
distance and phase, repeated ticks, logout/relog, completed saves, abandonment,
death, map/flight changes, failed spawn/path, static AI and cave handoff. The SQL
test applies the migration twice in temporary tables, checks the untouched native
objective and actual endpoint/guard/dialogue records, and verifies the follow-up
offer, summon, path and unrelated data.

## Remaining client validation

Accept 39374 at Oakin, follow him to the tent, hear the exchange, receive 1/1,
turn in and accept 39455. Repeat with two players, walking out during the reply,
logout/relog, death and abandon/reaccept. Confirm personal visibility, movement,
localized speech, questgiver icon and the cave transition in the 7.3.5 client.
These visuals and the full cave trial have not been verified in game.

The next separate audit is 39455's altar and Warmonger encounter. This change
preserves its existing script; it does not claim that encounter is verified.
Boost, ship tutorial, pet, camera and global scenario code are unchanged.
