# The Skies of Highmountain (39387): required ritual

The required Thunder Totem ritual now grants its witness credit after the player's own Lasan reaches Mayla and finishes his pledge. Meeting him, opening gossip repeatedly, another player's ceremony, or an interrupted attempt cannot complete the ritual.

## Confirmed defects and repair

- TDB735 objective 280358 is TALKTO 98773, storage 0. Objective 280508 also references 98773 but is the required witness, storage 1, native flags **3**. Objective 280509 references 99225, storage 2, native flags **12** (hidden and optional). The last two flags had been cleared locally. Restored the native flags; no replacement IDs were invented.
- Old SmartAI gossip immediately awarded generic 98773 credit, then waypoint 8 awarded the same credit to players within 40 yards. Waypoint 14 similarly awarded 99225 to bystanders. These rows are replaced by a script that addresses the three distinct objective IDs and credits only the owner.
- Phase 6626's conditions in zone 7503/entry 26 and zone 7731/entry 4 previously required the first 98773 objective to be incomplete. `GetQuestObjectiveData(quest,ObjectID)` returns the first matching objective, so meeting Lasan could remove his phase and prevent retry. Both phases now remain active while the quest is incomplete or complete, until reward.
- The static Lasan remains at the existing spawn 14641573, 4171.37 / 4398.75 / 660.976. Gossip 18944/0 creates a personal 98773 actor. Duplicate starts are rejected; failed spawns and interrupted attempts can be retried. Native event 47401/196066 is retained, with summoned-actor start-position, owner-distance and duplicate guards.
- The actor walks to existing waypoint 12, 4087.64 / 4379.32 / 670.626, within 10 m of Mayla's actual spawn. It requires an actual movement-arrival callback at that position, visible living Mayla 93826 (or 108434), and the nearby owner. The old final waypoint is over 15 m from the local Mayla spawn; it is not used as the interaction endpoint.
- Existing BroadcastText 100850/100852/100853 and emote 66 are retained. Added the missing native introduction 100851, also present in local hotfix `broadcast_text`. Dialogue is sent only to the owner. The script waits between lines and awards hidden wait then required witness after the final line. Delays are server tuning, not recovered retail timestamps.
- Abandon/reaccept expires the actor's quest lifetime. Death, map/phase changes, leaving range, missing Mayla or a path timeout cancel the attempt without witness credit. No global quest handler, level-100 boost, scenario, pet or mover path changed.

Primary contemporary capture confirms the meeting at Thunder Totem, pledge and Mayla handoff, and identifies Aviash as transport to the meeting: [Blizzplanet, 2015-12-14](https://warcraft.blizzplanet.com/the-skies-of-highmountain-highmountain-quests-alliance/). Exact objective flags and IDs are from the version-specific TDB archive; native spell/vehicle records and hashes are in `highmountain-skyhorn-ritual-native-2026-10-12.json`.

## Validation

- `Test-HighmountainSkyhornRitualNative.py`: native objective types, IDs, amounts, flags, both Mayla enders, RewardNextQuest 0; native ritual event and Aviash summon/vehicle/seat records.
- `Test-HighmountainSkyhornRitualData.ps1`: migration applied twice to temporary copies, native objective parity, script binding, no old credit rows, phase and gossip conditions, dialogue, actual actor positions and turn-ins; unrelated AI/objectives/conditions preserved. `-Live` verifies deployment.
- `Test-HighmountainSkyhornRitual.ps1`: compiles the actual production script and quest-lifetime handler with fixtures. Covers two owners, duplicate/repeated gossip, failed summon, real arrival, dialogue timing, no early or bystander completion, path timeout, missing Mayla, abandonment/reaccept and death/map/phase/range cancellation. Static and summoned instances share the bound production class.
- `Test-HighmountainSkyhornRitualGeometry.ps1`: actual Release navigation libraries and installed map 1220 data find a complete **23-polygon** ground route between the script endpoints.
- `Test-HighmountainBloodtotem.ps1`: actual core completion handler checks all eight objective subsets; meeting and witness are required, the hidden optional wait is not independently required.
- Release scripts compilation and canonical runtime deployment use `build-extractors/bin/Release`, with database/executable backup and protected-file checks.

Client playthrough, visible dialogue/emote timing and final reward remain unverified. These automated checks do not claim a retail-perfect scene.

## Separate transport finding for the next batch

**Aviash's optional ride is not repaired by this ritual patch.** Static 98777 at 5121.46 / 4461.57 / 1021.69 has spellclick 194359; native DB2 confirms this summons 98778 with properties 3054. That summon has vehicle 4428/seat 16028 but no AI/script, waypoint path, or automatic boarding/flight implementation. Summon properties use control 0, not the core's automatic vehicle-boarding category. This explains a summoned bird without a working ride.

There is no dedicated taxi node at the Haglands bird in the runtime DB2; Thunder Totem's regular node 1719 is on the upper platform and is not proof of the quest's flight route. Keep the existing IDs, recover or validate a route and implement boarding, transport and dismount as the next small batch. The repaired required ritual can be reached independently by traveling to Thunder Totem. Do not describe this whole quest's optional transport as verified.

Follow-up research recovered the explicitly named native landing point **WorldSafeLocs 5363** and confirmed ground navigation from it to Lasan. The original flight spline remains unresolved; see [Aviash research and reproducible checks](highmountain-aviash-2026-10-12.md).
