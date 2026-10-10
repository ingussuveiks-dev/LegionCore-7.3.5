# Val'sharah main-story audit, 2026-10-10

Scope: 46 quests from the Dalaran introduction through the archdruids, both faction variants, Tears of Elune (40890), and Darkheart Thicket (40567). This is a first repair batch, **not certification that the entire story is playable**.

## Evidence

- Local 7.3.5 TDB world snapshot, verified build 25549: quest definitions, 58 objectives, original reward items and quest enders. Exact input hash and extracted facts are in `valsharah-chain-native-2026-10-10.json`.
- Runtime `QuestLineXQuest.db2`: lines 184–190, including independent Horde line 189 and Alliance line 190. These establish story membership/order; they do not by themselves prove every unlock condition or scripted action.
- Runtime `SpellEffect`, `Item`, and `QuestPackageItem`: original click/teleport effects, three artifact-power items, and placement chest package 665. Deleted reward spells 181865 and 81040 are deliberately not restored from the older TDB snapshot.
- Current world data: existing Bramble Wall 242279 at (1720.89,6851.81,-0.56342), Morphael 91045, druid spell-clicks, original portal destinations, dungeon bosses/accessory and quest relations.
- Supplemental contemporary gameplay accounts: [Return to the Grove](https://www.wowhead.com/quest=38148/return-to-the-grove), [Tears of Elune](https://www.wowhead.com/quest=40890/the-tears-of-elune). Modern reward displays are not used as Legion item evidence.

## Repairs in this batch

- Separate the plant and Lyrathos counters for 38582: they previously shared storage index 2. Restore native storage/flags for 39384/40573 and native objective flags for 39383/38684.
- Restore hidden Bramble Wall objective 280418. Credit comes from using the existing door while on 38147. Morphael still needs an actual kill. The core skips this hidden objective as a completion requirement, matching its existing treatment of flag 16; the hook does not substitute for killing Morphael.
- Exclude native debug spell 197654 ("Val'sharah Playtest") from the quest loader's event-flag inference. Its nine mass-completion effects were reinstating event locks at startup; the spell itself remains unchanged. Ordinary completion spells still infer their required flags, covered by a compiled production-loop regression.
- Remove event-complete scripts and special event locks from quests already implemented by ordinary kills, spell-clicks or visiting their ender: 38142, 38381, 38382, 38384, 38225, 38235, 38147, 39384. Preserve the existing 40122 ride, its waypoint completion and phase refresh.
- Remove Aranelle as the wrong turn-in for Archdruid of the Claw. Add Koda's missing Return to the Grove starter, restore predecessor links for the archdruid tasks and require both barrow jobs.
- Remove 38377's mandatory 38323 prerequisite, leaving the existing alternative Return to the Grove conditions. Completing a different archdruid return no longer forces the Vale return as well. Full all-three-archdruid availability still needs a separate behavioral review.
- Restore Horde predecessors and shared Regroup/Reading the Leaves/Softening the Target paths; fix gossip conditions that previously required mutually exclusive Alliance and Horde quests simultaneously. Preserve the completed-vigil Tyrande phase for either faction.
- Remove Suramar's unrelated item 140758 and NPC 97140 turn-in from 43576; restore its Val'sharah metadata and keep its actual ender 103022.
- Restore reward items 141387 (38377), 141390 (38753), and 141383 (38743). Chest package 665 remains on placement 40890, rather than also being awarded for 38743.
- Tears of Elune now follows 38743 independently of optional dungeon 40567. Item 139043 is still provided on acceptance and removed through the existing quest item-drop/source-item cleanup on reward.
- Extend the existing portrait-room teleport handler to 40890. Quest credit 109750 is awarded only after successful teleport acknowledgment at the downstairs destination, with alive/map/quest guards. Remove the former upstairs/downstairs proximity credit. The existing Eye quest, reverse teleport and no-bounce behavior remain tested.
- Terminate Darkheart Thicket's door array: `LoadDoorData` scans until entry zero, so the former three-entry array read beyond its end.
- Detach Malfurion before destroying his cage. Fix the five-argument `MoveJump` call that selected the wrong overload: coordinates/orientation/speeds are now explicit. Exit gossip requires completed Xavius encounter, the actual menu option and an alive player outside combat/vehicles. Xavius quest credit remains normal boss-kill credit.
- Restore 38753's native post-movie handoff: spell 206723 triggers movie 473 and the search credit; after the movie, spell 218440 plays scene 1350/package 1676. Native scene text 14296 emits `TYRANDE`. The server now handles that event by spawning personal quest ender 102938. A delayed fallback and login/death/grid recovery restore the ender without granting objectives. Its recovery placement uses the existing 206723 destination, not a claimed retail spawn capture. Rewarding either Love Lost variant removes the personal ender. Restore missing Horde acceptance action 41054 -> companion summon 203477, matching existing Alliance 41056. Exact runtime evidence is in `valsharah-handoff-native-2026-10-10.json`.

## Existing character progress

The character migration runs once through the updater. It remaps 39384 and 40573 counters without discarding cave/cage progress. For 38582 the old shared counter cannot prove whether the player killed the boss. Plant progress remains in slot 2; an active, previously complete quest returns to incomplete and requires a real boss kill for the new slot 0. Already rewarded quests and unrelated quests are preserved. No rows for these quests existed in the live character objective table before this deployment.

## Validation

- `Test-ValsharahChainNative.py`: 46 native quest definitions, 58 objectives, both native faction quest lines, reward and spell evidence.
- `Test-ValsharahChainData.ps1`: actual world migration twice on temporary table copies; all 58 objective layouts, no overlapping counters, shared-faction conditions, removed shortcuts, rewards, item cleanup fields, custom bindings and existing ride preservation.
- `Test-ValsharahCharacterData.ps1`: actual character migration against fixture data; remapped counters, retained plant progress, no invented boss kill, rewarded/unrelated quests preserved.
- `Test-ValsharahChain.ps1`: compiled production wall handler, boss death/Malfurion methods and door array. Verifies interaction restrictions, no duplicate credit, detach-before-despawn, jump coordinates and exit-gossip restrictions. The same compiled test exercises the actual quest-loader inference loop against the playtest spell, a real completion spell and missing quests.
- `Test-EyeQuestChain.ps1`: existing dungeon/portal regression plus Tears-only pending/failed/successful teleports and idempotent credit.
- `Test-ValsharahHandoffNative.py`, `Test-ValsharahHandoff.ps1`, `Test-ValsharahHandoffData.ps1`: native movie/scene/event and companion effects, compiled production recovery handlers (including long death and leaving the arrival area), personal isolation/cleanup, and actual SQL migration twice on temporary table copies with both faction actions and custom binding preservation.
- Release build and post-start runtime checks recorded in the completion message. These tests do not validate client camera, NPC dialogue, visual effects or a complete playthrough.

## Remaining work discovered by this audit

1. **38687 / 41763, Close Enough to Touch:** repaired in the finale follow-up (`valsharah-finale-2026-10-10.md`). Removed automatic completion and bulk credit; actual following, personal native click actors, the native choice scene and guarded final credit now drive progress. Reconstructed placement/pathing and client scene timing require in-game QA.
2. **38743, The Fate of Val'sharah:** repaired in the finale follow-up. Gossip no longer awards victory. The acknowledged existing teleport leads to a personal current-map Ysera fight with native combat spells, actual defeat credit and movie 472. Retry and turn-in recovery preserve progress. No obsolete scenario criteria or replacement credit was invented. Combat balance and client presentation require in-game QA.
3. **38753 / Love Lost transition:** the missing native scene/server handoff and Horde companion acceptance action are now repaired. Client movie-to-scene timing, dialogue, visibility and following the companion still need a game run. The fallback guarantees only an available ender after the native search objective, not certification of retail scene choreography.
4. **38377 / later escorts:** server execution repaired in `valsharah-rituals-2026-10-10.md`: native ritual credit after its scene duration, owner-followed waypoints and personal death-gated vigil waves with cleanup/retry. Existing alternative Return to the Grove prerequisites are retained; an all-three requirement was not established. Client choreography/pathing remains unverified.
5. **In-game validation:** finish both faction routes; test real door collision, druid clicks, combat, transports, client objective display, dungeon escape, Tears turn-in UI and placed-pillar visibility. No game-client run occurred in this batch.

No boost tutorial, ship/Broken Shore scenario, shared criteria handling, player mover-control or pet lookup code was changed.
