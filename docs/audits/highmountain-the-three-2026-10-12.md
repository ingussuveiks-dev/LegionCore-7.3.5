# The Three (39321) and the parallel Haglands assignment

## Confirmed fixes

The Three and Assaulting the Haglands (39429) now both open after Rocs vs
Eagles (38910), and both must be rewarded before The Witchqueen (39322).
Previously 39429 required 39321, making the pair unnecessarily serial.
Both addons use negative exclusive group -39321 and point to 39322; the
production prerequisite handler requires every rewarded member of this
group. No unrelated incoming link to 39322 exists in the live database.

The [2016 WoW-Pro route](https://www.wow-pro.com/highmountain-neutral/)
records accepting both quests from the first Haglands Lasan after 38910,
doing them together, then handing in both before accepting 39322. TDB735
confirms the exact objectives, Lasan enders and titles. QuestLineXQuest is
display order, not sufficient proof of serial prerequisites; it is retained
as supporting evidence rather than used to invent an acceptance sequence.

Ragi 95194 and Agara 95195 used SmartAI with only an out-of-combat aura
event. SmartAI does not automatically schedule creature_template_spell.
Added combat events cast native area Hex 192111 and Deathsong 194228 on
self, matching their native source-area/self effect targeting. Initial
delay is three seconds, repeat 15/12 seconds respectively. These cadences
are server tuning, not recovered retail sniff timings. Existing Blood
Rights 213273 and Deathsinging 194245 idle events are preserved; SmartAI's
EnterCombat already interrupts their channels before combat events.

Following the spells beyond their first effects exposed two missing dummy
handlers. Neither spell_dummy_trigger, spell_scripts, spell_script_names
nor a hardcoded handler supplied the missing operation:

| Parent chain | Native dummy EffectBasePoints | Continuation |
| --- | --- | --- |
| Deathsong 194228 -> 194241 | 215588 | 215588 school damage |
| Foul Blast 191955 -> 215591 | 215592 | 215592 missile -> 215593 damage and knockback |

Added option-6 spell_dummy_trigger rows only for these two exact native
links, effect 0 and HIT_TARGET. This casts the child's own spell normally
as triggered and preserves its native damage. Using option 0 would instead
copy the dummy's large spell-ID value into damage; that is deliberately
not used. No spell IDs, damage values or substitute visuals are invented.

Ugla 95196 already uses the default hostile combat AI, which schedules her
existing Foul Blast 191955 entry. Her AI configuration is retained; the
missing downstream dummy link is repaired. All native references in these
chains, including Hex's 215583 continuation, exist. Hex's transformation
template 97277 (Hexxed Frog) exists with display 21950.

## Objectives, actors and handoff

- Native The Three objectives 279205/279216/279217 require one each of
  95194/95195/95196 at storage slots 0/1/2, flags 1/0. Minimap tracking
  flag 1 does not bypass completion.
- All three hostile targets are unphased on map 1220 in the Haglands:
  Ragi spawn 282902, Agara 282919 and Ugla 282860. Each has valid 98–110
  scaling. Existing positions and template flags are preserved.
- 39429 requires eight 95266 credits and six 95277 credits. Native aliases
  95265 and 105068 credit 95266. In the local Haglands there are 19
  Deathscreamers, 15 Hagfeathers and 13 Loyalists, so both objectives have
  available sources. Credit aliases match TDB735 and are unchanged.
- Lasan 95410 starts/ends both assignments and starts The Witchqueen.
  Upper Haglands spawn 340578 at 5238.11,4209.42,943.218 has phase IDs
  matching unconditional zone definition 24. This confirms a database phase
  route to the handoff, not a client visibility/pathfinding playthrough.
  The existing broader staging of Lasan's variants is unchanged.

## Validation

- `Test-HighmountainTheThreeNative.py` reads TDB735 and runtime DB2 records,
  verifies objectives/aliases/enders and the full native spell links above;
  the adjacent JSON records these records and source hashes.
- `Test-HighmountainTheThreeData.ps1` applies the migration twice in
  temporary tables, checks parallel gates, SmartAI abilities/idle events,
  exact dummy mappings and modes, all actors/scaling, mobs, turn-in and
  unrelated row preservation. `-Live` checks the deployed data read-only.
- `Test-HighmountainTheThree.ps1` compiles the production dummy-trigger
  filters and option-6 case. It reproduces absent links, rejects wrong
  handle/effect and out-of-world units, and verifies the correct child is
  cast once on each hit target without a damage override. Other dummy
  options and world spell execution are outside this fixture.
- `Test-HighmountainBloodtotem.ps1` exercises production kill-credit,
  completion and prerequisite code: all eight subsets and six kill orders
  of The Three, duplicate/absent/rewarded rejection, all four reward
  subsets for the parallel pair, and the native 8+6 objective including
  alternate harpy credit paths.

Release build and live startup checks accompany deployment to the canonical
runtime. Actual combat balance, Hex appearance, channel/missile visuals,
knockback and the full 7.3.5 client playthrough remain unverified. No boost,
ship scenario, generic pet/mover or warlock production paths are modified.
Next: The Witchqueen (39322), followed by the Skyhorn return/ritual.
