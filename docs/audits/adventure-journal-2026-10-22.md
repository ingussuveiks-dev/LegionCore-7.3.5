# Adventure Journal quest buttons — 2026-10-22

Client report: level-110 Maiko's Brawl: Gravity Lapse and Suramar Start Quest buttons both do nothing.

The native 7.3.5 DB2 reader against the canonical runtime reports AdventureJournal 270 (Suramar) Type=0, ButtonActionType=3, QuestID=39985, condition 43002 (level 110); 324 (Gravity Lapse) Type=5, ButtonActionType=3, QuestID=47148, condition 50416. The handler incorrectly required Type=3, confusing suggestion category with button action. Its preview also failed to authorize the corresponding self-offered quest in the existing accept handler.

Use ButtonActionType and native PlayerCondition admission. Authorize the exact offered quest with the existing popup ID. The shared start opcode now admits eligible journal quests separately from class-hall scouting; it does not consume the scouting selection or grant credit 97067. Repeated, already rewarded, wrong-action and ineligible requests retain the usual quest checks. Multiple journal suggestions sharing a quest are searched for an eligible entry.

Automated: Test-AdventureMapGossip.ps1 extracts and executes the production preview/start handlers, including both real category/action pairs, condition/take/log-space failures, repeat acceptance, completion and coexistence with a scouting quest. Existing fifteen-board flag admission and class-hall service tests pass.

Release verification: verify_highmountain_quest_release.py confirms the actual canonical PE compares the packed ButtonActionType field at offset 68 and contains the journal AddQuestAndCheckCompletion call path. Canonical worldserver reached ready without startup/database errors and was gracefully stopped after the owned hidden validation.

Pending: actual client opening/acceptance of both suggestions. Starting the brawl quest does not itself verify battleground queueing, the match or its reward.
