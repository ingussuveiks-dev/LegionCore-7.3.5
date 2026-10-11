# Highmountain: Rite of Blood (39860), 2026-10-11

This batch fixes the initiation and ownership of the existing Rite of Blood encounter. The earlier 39455 trial, level-100 boost, ship, global criteria, pet and movement handlers are unchanged.

## Evidence

- TDB735 (2018-02-19) identifies objective 280039 as monster credit 95259, followed by tracked/sequenced objective 280040 for killing 97418. Both require one. It identifies 96286 as the turn-in NPC.
- Runtime 7.3.5 SpellEffect/SummonProperties confirm 190902 -> Oakin 97440 (3686), 192536 -> Navarrogg 96286 (3699), 192682's portal aura, 212758's scripted sacrifice effect, 212762 -> conversation 2244, and Marakhan's 197670/197681 combat effects. Both companion summon properties have Control=1, Title=0, Slot=0 and Flags=8522506: ally guardians, not wild summons. The core's existing guardian/pet protections remain in effect.
- The native 192482/192484 interaction includes first-objective credit and a stun/scripted event. The old implementation depended on a spellclick; the documented objective calls for approaching Navarrogg. The replacement uses validated proximity and direct quest-specific first credit after successful scene allocation.
- Existing live marker 97675, GUID 283448, anchors the portal/boss at 5103.12, 5073.15, 627.398. Navarrogg 95259 is GUID 283395 at 5110.94, 5071.74, 627.359. No invented spawn coordinates or replacement IDs.
- [Wowhead 39860](https://www.wowhead.com/quest=39860/rite-of-blood), including the firsthand Boxofbeer playthrough description, corroborates approaching Navarrogg and fighting Marakhan. Modern reward/level data is not used to alter 7.3.5 records. TDB735 has no starter row for 39381 in this extract; the existing live 96286 starter and 39860 prerequisite are preserved and tested.

## Defects and changes

The shared SmartAI route toggled a shared click flag, blocked on any nearby Marakhan, summoned a non-player-owned boss, selected the nearest Oakin for sacrifice, and selected a nearby player for combat. Those choices could mix simultaneous players' events and leave an abandoned encounter blocking the next attempt.

- A live player on 39860 starts by approaching Navarrogg within six yards with matching phase, visibility and line of sight. A failed allocation gives no objective credit. A live private boss prevents repeat initiation.
- Each player gets a private portal and boss. Oakin is restored through the original native summon spell for retries. Pending-removal summons are excluded from lookup, because actual TempSummon removal is queued.
- The existing scene's portal delay, two lines, sacrifice spell, and combat delay are retained. Only the initiating player's saved Oakin GUID is targeted. The shared marker no longer executes a second SmartAI encounter.
- The boss remains protected during the introduction. Afterwards, the owner and their pets can fight it. AggressorAI retains the existing combat spells and ordinary melee. Nearby unrelated players cannot attack or be selected as the scripted target. Assistance/flee behavior cannot recruit unrelated creatures.
- No timer, spawn, dialogue or sacrifice grants final credit. Marakhan must actually die; ordinary KillRewarder grants the native objective after 50 ms. Its normal group reward rules remain unchanged. The corpse is retained for 30 seconds, and the guide recovery runs afterwards, at 100 ms.
- Death, abandonment/reacceptance, phase/range loss, evade, logout and map changes cancel the old attempt. Quest-lifetime checks prevent an old scene from progressing a reaccepted quest. Cleanup uses saved actor GUIDs. A new approach retries the encounter without resetting recorded first-objective progress.
- Native spell_area guide creation, guide SmartAI conversation/follow/escape, turn-in and 39381 gate are preserved. Owner-only guide lookup additionally restores a missing guide on login or approach for completed 39860 (or rewarded 39860 before completing 39381).
- Marakhan's two garbled fallback strings are restored from local hotfix BroadcastText 99494 and 82906; existing localized IDs remain intact.

## Validation

- `Test-HighmountainBloodRite.ps1`: compiled actual production handlers, ordered scene, no premature final credit, two-player isolation, correct sacrifice target, pet eligibility, failed summons including queued removal, delayed kill/guide ordering, duplicate prevention, guide recovery and death/evade/logout/map/abandon retries.
- `Test-HighmountainBloodRiteNative.py`: TDB735 objectives/ender and runtime DB2 effects/properties, with reproducible hashes in the adjacent evidence JSON.
- `Test-HighmountainBloodRiteData.ps1`: migration applied twice to temporary copies, preserved ordered objectives/combat/scaling/companion/guide/escape/next gate, and unrelated data checks. `-Live` verifies deployed bindings.
- Existing Blood Trial and Audience live-data regressions, `Test-PetLoadSelection.ps1` and `Test-WarlockRegression.ps1` pass.
- Full Release build and startup are checked in the canonical `build-extractors/bin/Release` directory; deployment details are recorded in the task result.

## Client verification still required

No 7.3.5 client playthrough is available in this session. Verify the portal model, sacrifice animation, camera/stun presentation, boss spell visuals, permanent pet portrait, guide conversation, turn-in and subsequent escape in game. This is a functional repair of the existing scene, not a verified reconstruction of every retail embellishment: the documented rune, assisting NPCs and extra Feltotem waves were not present in this implementation and are not fabricated here. Scene timings are inherited from the existing SmartAI, not claimed as measured retail timings.
