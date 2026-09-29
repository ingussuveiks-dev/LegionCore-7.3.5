# Broken Shore reentry, 2026-09-29

The login check unconditionally rejected scenario maps. On map 1460 it now
allows a character whose faction's Battle for the Broken Shore quest is still
incomplete (42740 Alliance, 40518 Horde), then runs the usual access checks.
Saved dynamic ship GUIDs are cleared so the instance can place the passenger
on its current transport.

Startup keeps one-member LFG groups, removes orphan LFG rows, and restores
their dungeon, instance category and non-selectable scenario/LFR difficulty.
The group query also supplies the real legacy raid difficulty instead of
reading the group storage ID as a difficulty.

Holgar Stormaxe, Recruiter Lee, Captain Angelica and Captain Russo offer
**Return me to the Broken Shore.** after the character has embarked and while
the introduction quest remains incomplete. Existing LFG instances are reused;
otherwise the normal slot-908 queue starts a fresh instance. Returning to an
already landed instance uses the current stage's graveyard instead of the
ship-local LFG entry coordinates. Normal relogging preserves the saved spot.

Scenario stages remain in memory. A destroyed instance or server restart
starts the island sequence again; this change does not serialize the island's
waves, dialogue or criteria. The recovery option allows the quest to be
finished again through the scenario's normal completion flow.

Custom creature 230005 had six spawns and a server template but no WDB row in
either the live database or the archived source dump. World update 326 adds a
neutral Service NPC name and display 308, also used by custom Quest Repair
230006. Existing nonzero models are preserved when the update is reapplied.

Validation:

- Release worldserver built directly in `build-extractors/bin/Release`.
- `tools/tests/Test-BrokenShoreReentry.ps1` compiles extracted production
  login, group-loading, return-menu and entrance-recovery handlers. It covers
  both factions, finished/missing quests, normal departure, invalid selections,
  live-instance reuse and stage-specific entrance placement.
- The same runner executes the production startup cleanup SQL against
  connection-local temporary tables, including solo LFG, ordinary solo,
  empty groups, missing characters and orphan binds/LFG rows.
- World update 326 was applied twice successfully to test idempotence. Holgar's
  binding and the model row were checked; all six service spawns remain.
- The current runtime DB2 contains display 308 and all ten Broken Shore
  graveyards used by the script, each on map 1460.
- The initial Release server reached ready with zero startup ERROR lines.
  Ports 8085 and 8086 both sent their server greeting. That validation server
  and its hidden supervisor were subsequently stopped at the user's request.

Client validation is still required: return as Maiko, relog on the island,
restart and reenter, then finish the introduction normally. The first client
retest exposed the Dalaran regression described below. No direct database
quest-completion edits were made to Maiko.

## Dalaran regression from the first client retest

Holgar also has SmartAI listening for menu 20487, option index 0, to run the
introduction skip chain and cast 230156. The return option originally used the
automatic option allocator. With database option 0 hidden by its account-quest
condition, the return option occupied index 0. NPCHandler invokes SmartAI before
the C++ gossip handler, so both skip and return ran on the same click.

Recovery now uses explicit option index 99. The regression test compiles the
production GossipMenu allocator and SmartScript gossip event handler along
with the return handler: it reproduced the skip action before the fix and
checks that recovery triggers no skip action with the original skip option
either hidden or visible. The original visible skip option still works.

World update 327 binds the same recovery script to custom creature 230005,
renamed Broken Shore Guide. Its existing Krasus' Landing spawn provides a
return route for characters already stranded in Dalaran; its existing capital
spawns offer the same option. No new spawn or automatic quest completion is
added. The recovery handler logs the player's request for client retests.

The corrected Release executable was linked in a staging directory and copied
to `build-extractors/bin/Release/worldserver.exe`; its SHA-256 is
`5604E3BBC04932D85D24F4CC4F7878DC2EE1DECBDB8C552507A246F8FD431B97`.
Update 327 was applied twice, and the extracted-handler and temporary-table
tests passed. The corrected executable has not been started for client
validation. Both server processes were absent at handoff.

## Faction quest correction

The boost departure originally assigned the faction quest IDs in reverse.
Alliance quest 42740 ends with Genn Greymane (100395); Horde quest 40518 ends
with Eitrigg (100453), who offers Fate of the Horde (40522). Boost departure,
scenario login and the return menu now use these same correct IDs. The
regression runner compiles the actual boost quest selection for both factions.

A Horde GM character that already completed the scenario with the wrong quest
can preserve that completion without replaying the scenario: select the
character, remove quest 42740, add quest 40518 and complete quest 40518 using
the GM quest commands. Then turn it in normally to Eitrigg. This replaces the
mistaken quest; it does not grant its reward or mark the rest of the chain
complete. No live character database rows are edited by this fix.

The quest selection change does not alter sparring waves, damage handling,
vehicle boarding, departure flight or disembarkation. A new boost still needs
the full client sequence checked after installing the corrected Release build.
