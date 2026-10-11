#include "ScriptMgr.h"
#include "Player.h"
#include "QuestData.h"
#include <algorithm>

class player_bloodtotem_objectives : public PlayerScript
{
public:
    player_bloodtotem_objectives() : PlayerScript("player_bloodtotem_objectives") { }

    void OnLogin(Player* player) override
    {
        auto status = player->GetQuestStatus(39373);
        if (status != QUEST_STATUS_INCOMPLETE && status != QUEST_STATUS_COMPLETE)
            return;

        // Reconcile the former 30-feather completion with the restored native
        // requirement. Only carried items count towards a collection objective.
        if (Quest const* quest = sQuestDataStore->GetQuestTemplate(39373))
        {
            for (QuestObjective const& objective : quest->GetObjectives())
                if (objective.Type == QUEST_OBJECTIVE_ITEM && objective.ObjectID == 128380)
                {
                    player->SetQuestObjectiveData(quest, &objective,
                        std::min(player->GetItemCount(128380, false), uint32(objective.Amount)));
                    if (status == QUEST_STATUS_COMPLETE && !player->HasQuestObjectiveComplete(quest, objective))
                        player->IncompleteQuest(39373);
                }

            if (player->CanCompleteQuest(39373))
                player->CompleteQuest(39373);
        }
    }
};

void AddSC_highmountain_bloodtotem()
{
    new player_bloodtotem_objectives();
}
