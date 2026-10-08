# Character spell effect chains in Legion 7.3.5

The 40 non-passive class candidates without a direct visual mapping were
checked against the local 26972 DB2 files, SQL dispatch tables and C++
handlers. One gameplay defect was reproduced and repaired: Death Coil's
script and the default SQL dummy dispatcher both cast its damage payload.

## Death Coil repair

Spell 47541 has a dummy effect. Its registered `spell_dk_death_coil` handler
casts damage spell 47632 and, for an Unholy player with a pet, energy spell
196263. The installed `spell_dummy_trigger` table also contains both payloads
for effect mask 1 and handle mask 8 (HIT_TARGET). `Spell::HandleEffects` runs
the script before the default `EffectDummy`, so leaving the default enabled
dispatches the payload again. Spell 47632 owns the missile visual 38742.

The script now calls `PreventHitDefaultEffect(effIndex)` after validating the
caster and target. SQL remains available as a fallback when the script is not
bound; it no longer runs after the script handles a normal hit.

`Test-CharacterSpellChains.ps1` compiles the extracted production handler.
The test failed before the change and passes afterward. It includes the SQL
damage fallback and checks one damage cast, one eligible pet-energy cast,
no pet, another specialization, a non-player caster and absent targets.

The earlier audit incorrectly called aura 77 a language aura. That text is
corrected: it is `SPELL_AURA_MECHANIC_IMMUNITY`, used by Honorable Medallion's
core crowd-control removal handler.

## Review of all 40 candidates

“Native” below identifies a native effect or presentation mechanism; it does
not claim that the client rendered it correctly during this audit. Related
parent presentation spells are distinguished from casts made by the candidate.

| Spell | Presentation or effect path | Result |
| --- | --- | --- |
| 75 Auto Shot | Native weapon-damage effect and ranged attack presentation | No direct visual added |
| 1706 Levitate | SQL linked cast 111759 | Visual 38938 exists |
| 1784 Stealth | Core aura handling adds 158185 | Visual 39168 exists |
| 5019 Shoot | Native wand weapon-damage effect | No direct visual added |
| 10846 First Aid | Trade-skill and skill effects | No particle effect inferred |
| 36554 Shadowstep | DB2 trigger 36563 | Visual 39184 exists |
| 46584 Raise Dead | Conditional SQL casts 52150, 196910, 212027 | All three presentation paths exist |
| 47540 Penance | C++ selects 47757 or 47758; DB2 triggers healing/damage ticks | Channel and tick visuals exist |
| 47541 Death Coil | C++ casts 47632 and eligible pet-energy spell 196263 | Duplicate default dispatch fixed |
| 52610 Savage Roar | Core aura handler applies 62071 in cat form and removes it on expiry | Visual 38479 exists |
| 53428 Runeforging | Trade-skill effect | No particle effect inferred |
| 53478 Last Stand | Native health-increase aura | No missing direct DB2 reference |
| 55095 Frost Fever | Periodic-damage payload; related Howling Blast 49184 owns presentation | Parent visual 66692 exists |
| 55709 Heart of the Phoenix | C++ casts resurrection 54114 and lockout 55711 | Visual 39478 exists |
| 68996 Two Forms | C++ toggles altered-form aura 97709 and requests its transition | Model transition, not a missing particle row |
| 69046 Pack Hobgoblin | Native summon of creature 36613 | Creature presentation; no direct spell visual inferred |
| 81782 Power Word: Barrier | Protective aura inside parent spell 62618's area | Parent visual 67858 exists |
| 88163 Attack | Attack dummy entry | No extra particle effect inferred |
| 106832 Thrash | Action-bar override auras 48629 / 106829 select 106830 / 77758 | Cat and bear visual mappings exist |
| 107428 Rising Sun Kick | DB2 trigger 185099 | Visual 39941 exists |
| 109132 Roll | C++ applies and casts 107427 | Visual 22459 exists |
| 115546 Provoke | C++ selects 116189 or statue taunt 118635 | Both use visual 56431 |
| 119650 Energy Usage | Dummy aura, empty client description | Internal candidate; no replacement inferred |
| 119898 Command Demon | Pet aura 119904 receives its replacement spell in custom basepoints | Pet-specific wrappers and their payloads exist |
| 147362 Counter Shot | Native interrupt effect | Client supplies no direct visual mapping |
| 161211 Garrison Enchanting | Trade-skill and skill effects | No particle effect inferred |
| 185901 Marked Shot | SQL dummy cast 212621; C++ handles marks and set-bonus repeats | Payload visuals exist; no unconditional C++ duplicate |
| 190780 Frost Breath | Damage/slow payload associated with 190778 | Parent visual 49378 exists |
| 190925 Harpoon | SQL casts 190927 / 186260, with optional delayed Posthaste | Movement/visual payload exists |
| 195710 Honorable Medallion | Mechanic immunity with special core crowd-control removal | Corrected previous aura description |
| 198304 Intercept | C++ friendly/enemy selection, including Blazing Trail | Charge and protection visual mappings exist |
| 199736 Find Treasure | Native resource tracking aura | UI effect; no particle row inferred |
| 200163 Throwing Axes | C++ schedules 200167 at 500 ms intervals | Visual 52012 exists |
| 201078 Snake Hunter | C++ restores Mongoose Bite charges | Resource operation; no missing visual link established |
| 204035 Bastion of Light | C++ restores Shield of the Righteous charges | Resource operation; no missing visual link established |
| 206505 A Murder of Crows | Periodic C++ casts 131900, 131637, 131951, 131952 | Damage and crow visual mappings exist |
| 213764 Swipe | Action-bar override auras 48629 / 106829 select 106785 / 213771 | Cat and bear visual mappings exist |
| 215769 Spirit of Redemption | Native shapeshift aura, form 32, plus immunity/healing effects | Form presentation rather than a direct visual row |
| 219432 Rage of the Sleeper | Retaliation damage from the parent aura's C++ handler | Parent 200851 owns the visual mappings |
| 228545 Shapeshift Form | Dummy entry with empty description | Internal candidate; no speculative replacement |

## Visual dependencies and client files

The reviewed paths reach 59 SpellVisual records, 98 SpellVisualKit records,
101 model attachments and 76 SpellVisualEffectName records. Nonzero visual,
kit, fallback-kit, model-attachment, low-definition attachment, missile-set
and effect-name references in these paths all resolve.

The SpellVisualEffectName field order was checked against the 26972 layout
B930A934 in [WoWDBDefs](https://github.com/wowdev/WoWDBDefs/blob/master/definitions/SpellVisualEffectName.dbd).
Kit effect type 2 identifies model attachments in the
[kit-effect schema](https://github.com/wowdev/WoWDBDefs/blob/master/definitions/SpellVisualKitEffect.dbd).
The script uses the pinned layout hashes, including copy records and compressed
fields; it does not reinterpret these files as current retail layouts.

All 74 distinct directly referenced model/texture FileDataIDs were opened and
fully read from the local client's CASC archive using the repository CascLib.
The client's `.build.info` identifies 7.3.5.26972. All reads returned the full
reported file size with success. This does not verify additional dependencies
inside those files or their appearance when rendered. Sound, camera and
procedural kit effects remain outside this dependency check.

## Build and runtime

Release `worldserver` and `bnetserver` built successfully in the canonical
`build-extractors/bin/Release` directory. The regression test and
`Test-WorldShutdown.ps1 -ProbeConnections` passed, including current-run
readiness, both socket greetings, clean exit and no crash dumps. The only
DBErrors entry during this startup was the already-known orphan waypoint
script 347, unrelated to spell effects. No client casting session was run.

## Reproduction

First generate `audit_spell_visuals.py` output and the SQL exports described
in the [preceding audit](spell-visual-audit-2026-10-08.md). Retain these export
names: `visual-hotfix.tsv`, `visual-hotfix-spells.tsv`. In the same directory,
save the following queries as header-bearing TSV files:

| File | Database and query |
| --- | --- |
| chain-hotfix-effects.tsv | hotfixes: `SELECT * FROM spell_effect;` |
| chain-script-bindings.tsv | world: `SELECT spell_id, ScriptName FROM spell_script_names;` |
| chain-linked-spells.tsv | world: `SELECT * FROM spell_linked_spell;` |
| chain-spell_dummy_trigger.tsv | world: `SELECT * FROM spell_dummy_trigger;` |
| chain-spell_pet_auras.tsv | world: `SELECT * FROM spell_pet_auras;` |

```powershell
python tools/research/audit_character_spell_chains.py --audit audit.json --sql-directory exports --output chains.json
tools/tests/Test-CharacterSpellChains.ps1
```

The generated JSON includes DB2 hashes, spell paths, bindings, effect records,
visual dependency counts and all 74 direct FileDataIDs. `REVIEWED_PATHS` is an
explicit record of this source review, including parent presentation paths;
it is not an automatic proof that every listed spell casts every related spell.
