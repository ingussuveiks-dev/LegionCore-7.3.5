# Accepting quests from a multi-quest NPC

The user reported that accepting the first quest from an NPC offering two left the panel open and required closing it by hand. The earlier quest-detail flag ordering fix from `18fba3366` is still present in the current source and linked executable. There was no packet capture for this report.

Since `2d6fe335a`, `SendPreparedGossip` has sent quest-only lists with multiple entries through the general gossip message. The 26972 TrinityCore reference at `c2d61e05cf` uses `SMSG_QUEST_GIVER_QUEST_LIST_MESSAGE` for that case. Restore the NPC quest-list route through the existing `SendPreparedQuest` and packet writer. A condition on the greeting continues to hide the greeting and its emote. Eligible quests remain available. NPCs with dialogue options still use the existing combined menu. Quest admission and the acceptance close packet remain on their existing paths.

The Release game library was rebuilt and linked into a separate, non-incremental worldserver image. The final link used the rebuilt `game/Release/game.lib`, and the linker map records both changed functions in their rebuilt objects. The staged executable SHA-256 is `74F1FCEEB61DF3A479639992B9EC00B9B43308C5A64CE0CFB432B9EE07435CCA`. The active DBErrors log was empty and there were no crash reports during inspection. No separate test server or client automation was run for this live report.

The user-owned worldserver was running at staging. Deployment and visual confirmation remain open. The first client check is to select and accept one quest at an NPC with a quest-only list, then check whether the accepted quest panel dismisses and the next quest can be selected.
