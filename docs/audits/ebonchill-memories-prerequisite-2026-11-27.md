# Memories of Ebonchill quest order

The player had accepted Memories of Ebonchill (`42429`) with no Ebonchill (`128862`) and before starting The Mage Hunter (`42479`). The live world database had an empty `quest_template_addon.PrevQuestID` for `42429` and no incoming `NextQuestID` or quest acceptance condition. The quest is offered and turned in by Old Fillmaff (`107452`). The Legion quest text assumes the player already has the staff, and the recorded quest order places Memories after The Mage Hunter. [Memories of Ebonchill](https://warcraft.wiki.gg/wiki/Memories_of_Ebonchill)

The migration sets the positive predecessor `42479` on `42429`. `QuestData` loads that predecessor and `Player::SatisfyQuestPreviousQuest` requires its reward receipt. The Mage Hunter has no exclusive group. Its next-quest link and the existing equipment and phase checks remain in place. The SQL updates only the empty prerequisite and has no character writes. Players who already accepted Memories may keep it and complete it after obtaining the staff.

The current server was still running during the source and database checks. The normal updater will apply the migration at the next start. The next client checks are the offer after The Mage Hunter and completion of the existing active Memories quest. No separate startup or automated gameplay test was run during the user-led playthrough.
