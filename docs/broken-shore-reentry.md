# Broken Shore reentry, 2026-09-29

The login check unconditionally rejected scenario maps. On map 1460 it now
allows a character whose faction's Battle for the Broken Shore quest is still
incomplete (40518 Alliance, 42740 Horde), then runs the usual access checks.
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
