# Stormwind personal graveyard selection

The player released in Stormwind Harbor and appeared at the Spirit Healer on the boat below Lion's Rest. The matching 26972 `WorldSafeLocs.db2` identifies 5748 at `-8691.04, 1102.64, 0.7` as the personal graveyard for The Imposter. The live Alliance link under Stormwind zone 1519 admitted all Alliance classes. The phased boat actors near this point use phase 7434, which Stormwind phase definition 100 controls for the Rogue quest chain containing 37689.

Nearest-graveyard selection now admits 5748 on its own map only when the requester is a Rogue in phase 7434. Release spirit, both lookups during Spirit Healer resurrection, and the GM nearest-graveyard inquiry pass the player context. The other graveyard links and distance comparisons keep their existing rules. The native boat location and its Alliance link remain available to eligible Rogues.

## Observed and pending checks

- Loaded the matching native graveyard catalog and read the live graveyard link, phase definition, phase conditions, and surrounding actor records. Reviewed the release and resurrection paths. The closest public same-map location to the observed harbor position is Stormwind Cemetery, 1763.
- Rebuilt the Release game and scripts libraries and checked the final linker inputs. The map resolves the player-aware selector to `game:ObjectMgr.obj` and retains the earlier gryphon `PassengerBoarded` implementation. The Release executable SHA-256 is `88ABDA67FD87BFC189E4F493407A91F4C559ABBD86E9B977CF7510FDF4B5BFFA`.
- The death screenshot followed login after the previous gryphon crash. Before login the saved position was in the air near waypoint 5, with no saved transport. The death itself had no damage-cause log. Live read-only records later confirmed 40517 rewarded and the player alive in Legion Dalaran. Those records do not establish which flight or route the player used after resurrecting.
- User-led client verification is pending for a public Stormwind spirit release and an eligible Rogue release. No independent runtime or automated gameplay test was started. The period after the gryphon deployment had no newly appended warnings, errors, assertions or `DBErrors.log` entries and no new crash files. The previous gryphon crash files were absent from the active directory and were not found under `fixed` by filename.
