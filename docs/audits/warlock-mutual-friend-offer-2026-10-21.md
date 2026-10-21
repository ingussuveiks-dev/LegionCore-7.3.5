# A Mutual Friend unavailable, 2026-10-21

Maiko (Warlock, level 106) reported no available quest while in Dalaran's
Underbelly. The screenshot shows that map; the character's saved location during
the investigation was map 1502, zone 8392. The earlier advice identified the next
quest but had not checked the executable offer. This corrects that omission.

Read-only character checks confirmed rewarded 40824 (The Path of the Dreadscar),
no active quests and no reward for 44099/42608. All three artifact acquisition
quests were already rewarded. No character status was edited for this repair.

## Native route and failure

spell_area's existing row is spell 224338, area 7581, start quest 40824 with
status mask 64, end quest 44099 with mask 74, classmask 256, autocast 1.
The core's actual SpellArea::IsFitToRequirements interprets both masks as
allowed quest statuses. Mask 74 contains COMPLETE, INCOMPLETE and REWARDED;
it excludes NONE. Thus a character who has not accepted 44099 cannot receive
the messenger even after going to the correct area. This is the same mask
mistake previously fixed for the separate first Warlock introduction 40716.

The native C++ DB2 reader confirms area 7581 is Dalaran City on map 1220,
parent 7502. Dalaran Sewers is area 8392 with a separate parent. Spell 224338
contains the native summon of Black Harvest Acolyte 112693 and a marker aura.
The acolyte has the questgiver flag, follows its summoner, becomes personal,
and despawns five seconds after accepting 44099. It is not a permanent NPC
standing in the basement. Quest 44099 leads to Calydus; quest 42608 has native
minimum level 101 and predecessor 44099. Its availability has not been
client-verified by this change.

## Fix and verification

Migration 2026_10_21_00 changes only the legacy 74 end mask on this exact native
Warlock row to 1 (NONE). It preserves the rewarded prerequisite, city boundary,
class restriction, summon and existing dialogue/actions. It does not broaden
the trigger into the sewers or class hall, edit the Priest's adjacent row,
replace custom masks or complete quests for the player.

- Test-DalaranClassIntro.ps1 now runs the existing introduction and the later
  campaign offer through extracted production SpellArea and SetQuestUpdate.
  Covers every relevant prerequisite/quest-state combination, old-mask failure,
  city/sewer/class-hall boundaries, other classes, acceptance removal,
  abandonment retry, completion and duplicate marker suppression.
- Test-MutualFriendOfferData.ps1 applies the migration twice in connection-local
  temporary tables and checks the real giver/flags, follow/personal actions,
  acceptance despawn, next quest prerequisite and custom/other-row preservation.
  Its native DB2 catalog also validates the area and summon/marker records.
- Both tests passed. This is a database repair; the production C++ executable
  does not change and no server rebuild is required.

The migration was applied to the live world database after a scoped backup.
The user's running server was not stopped. Its loaded spell_area cache still
requires `.reload spell_area` (or a later normal restart). The reload command
is registered in cs_reload.cpp and calls LoadSpellAreas. After reload, leave
the Underbelly and enter Dalaran City to trigger the offer. Actual messenger
appearance, quest acceptance, turn-in at Calydus and the follow-up remain client
checks; SQL/native fixtures alone do not verify those interactions.
